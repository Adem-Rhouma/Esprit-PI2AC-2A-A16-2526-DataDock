import json
import os

import cv2
import numpy as np
from flask import Flask, jsonify, request
from flask_cors import CORS
from werkzeug.exceptions import RequestEntityTooLarge


HOST = "127.0.0.1"
PORT = 5000
DEBUG = True
MAX_CONTENT_LENGTH = 5 * 1024 * 1024
ALLOWED_EXTENSIONS = {"jpg", "jpeg", "png", "bmp"}
MATCH_THRESHOLD = 1.128
MATCH_METRIC = cv2.FaceRecognizerSF_FR_NORM_L2
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODELS_DIR = os.path.join(BASE_DIR, "models")
FACE_DETECTOR_CANDIDATES = [
    "face_detection_yunet_2023mar.onnx",
    "face_detection_yunet_2023mar_int8.onnx",
    "face_detection_yunet_2023mar_int8bq.onnx",
]
FACE_RECOGNIZER_CANDIDATES = [
    "face_recognition_sface_2021dec.onnx",
]


app = Flask(__name__)
app.config["MAX_CONTENT_LENGTH"] = MAX_CONTENT_LENGTH
CORS(app)


class FaceApiError(Exception):
    def __init__(self, message, status_code=400, **extra):
        super().__init__(message)
        self.message = message
        self.status_code = status_code
        self.extra = extra


def json_error(message, status_code=400, **extra):
    payload = {"success": False, "message": message}
    payload.update(extra)
    return jsonify(payload), status_code


def find_model_path(candidates):
    for filename in candidates:
        path = os.path.join(MODELS_DIR, filename)
        if os.path.exists(path):
            return path
    return None


def get_face_detector():
    detector_model = find_model_path(FACE_DETECTOR_CANDIDATES)
    if detector_model is None:
        raise FaceApiError(
            "Modele detecteur manquant : ajoutez un fichier YuNet dans le dossier models.",
            status_code=500,
        )

    try:
        return cv2.FaceDetectorYN.create(
            detector_model,
            "",
            (320, 320),
            score_threshold=0.9,
            nms_threshold=0.3,
            top_k=5000,
        )
    except AttributeError:
        raise FaceApiError(
            "OpenCV ne contient pas FaceDetectorYN. Installez opencv-contrib-python.",
            status_code=500,
        )


def get_face_recognizer():
    recognizer_model = find_model_path(FACE_RECOGNIZER_CANDIDATES)
    if recognizer_model is None:
        raise FaceApiError(
            "Modele de reconnaissance manquant : ajoutez models/face_recognition_sface_2021dec.onnx.",
            status_code=500,
        )

    try:
        return cv2.FaceRecognizerSF.create(recognizer_model, "")
    except AttributeError:
        raise FaceApiError(
            "OpenCV ne contient pas FaceRecognizerSF. Installez opencv-contrib-python.",
            status_code=500,
        )


def decode_image():
    if "image" not in request.files:
        raise FaceApiError(
            "Aucune image envoyee. Utilisez le champ multipart 'image'.",
            status_code=400,
            face_detected=False,
            quality_ok=False,
        )

    file_storage = request.files["image"]
    if not file_storage or file_storage.filename == "":
        raise FaceApiError(
            "Aucune image envoyee. Le fichier est vide ou sans nom.",
            status_code=400,
            face_detected=False,
            quality_ok=False,
        )

    extension = file_storage.filename.rsplit(".", 1)[-1].lower() if "." in file_storage.filename else ""
    if extension and extension not in ALLOWED_EXTENSIONS:
        raise FaceApiError(
            f"Extension d'image non supportee ({extension}).",
            status_code=400,
            face_detected=False,
            quality_ok=False,
        )

    file_bytes = file_storage.read()
    if not file_bytes:
        raise FaceApiError(
            "Image invalide : le fichier recu est vide.",
            status_code=400,
            face_detected=False,
            quality_ok=False,
        )

    image = cv2.imdecode(np.frombuffer(file_bytes, dtype=np.uint8), cv2.IMREAD_COLOR)
    if image is None:
        raise FaceApiError(
            "Image invalide : impossible de decoder le fichier.",
            status_code=400,
            face_detected=False,
            quality_ok=False,
        )

    return image


def extract_single_face_embedding():
    bgr_image = decode_image()
    detector = get_face_detector()
    recognizer = get_face_recognizer()

    height, width = bgr_image.shape[:2]
    detector.setInputSize((width, height))
    _, faces = detector.detect(bgr_image)

    if faces is None or len(faces) == 0:
        raise FaceApiError(
            "Aucun visage detecte dans l'image.",
            status_code=422,
            face_detected=False,
            quality_ok=False,
        )

    if len(faces) > 1:
        raise FaceApiError(
            "Plusieurs visages detectes. Veuillez fournir une image avec un seul visage.",
            status_code=422,
            face_detected=True,
            quality_ok=False,
        )

    face = faces[0]
    try:
        aligned_face = recognizer.alignCrop(bgr_image, face)
        feature = recognizer.feature(aligned_face)
    except cv2.error:
        raise FaceApiError(
            "Le visage a ete detecte mais l'embedding n'a pas pu etre extrait.",
            status_code=422,
            face_detected=True,
            quality_ok=False,
        )

    return np.array(feature.flatten(), dtype=np.float64)


def parse_reference_embedding():
    embedding_raw = request.form.get("embedding")
    if not embedding_raw:
        raise FaceApiError(
            "Embedding de reference manquant dans la requete /face/verify.",
            status_code=400,
        )

    try:
        embedding = json.loads(embedding_raw)
    except json.JSONDecodeError as exc:
        raise FaceApiError(
            f"Embedding de reference invalide : JSON incorrect ({exc.msg}).",
            status_code=400,
        )

    embedding_array = np.array(embedding, dtype=np.float64)
    if embedding_array.ndim != 1:
        raise FaceApiError(
            "Embedding de reference invalide : il doit etre un tableau a une dimension.",
            status_code=400,
        )

    return embedding_array


@app.errorhandler(RequestEntityTooLarge)
def handle_file_too_large(_error):
    return json_error(
        "Image trop volumineuse. Reduisez la taille du fichier puis reessayez.",
        status_code=413,
        face_detected=False,
        quality_ok=False,
        embedding=None,
    )


@app.route("/face/register", methods=["POST"])
def register_face():
    try:
        embedding = extract_single_face_embedding()
        return jsonify(
            {
                "success": True,
                "message": "Inscription faciale prete. Embedding extrait avec succes.",
                "face_detected": True,
                "quality_ok": True,
                "embedding": embedding.tolist(),
            }
        ), 200
    except FaceApiError as exc:
        return json_error(
            exc.message,
            status_code=exc.status_code,
            face_detected=exc.extra.get("face_detected", False),
            quality_ok=exc.extra.get("quality_ok", False),
            embedding=None,
        )
    except Exception:
        return json_error(
            "Erreur interne inattendue lors de l'inscription faciale.",
            status_code=500,
            face_detected=False,
            quality_ok=False,
            embedding=None,
        )


@app.route("/face/verify", methods=["POST"])
def verify_face():
    try:
        reference_embedding = parse_reference_embedding()
        current_embedding = extract_single_face_embedding()
        recognizer = get_face_recognizer()
        distance = float(recognizer.match(current_embedding, reference_embedding, MATCH_METRIC))
        is_match = distance <= MATCH_THRESHOLD

        return jsonify(
            {
                "success": True,
                "match": is_match,
                "distance": round(distance, 6),
                "threshold": MATCH_THRESHOLD,
                "message": (
                    "Verification faciale reussie : correspondance confirmee."
                    if is_match
                    else "Verification faciale effectuee : le visage ne correspond pas."
                ),
            }
        ), 200
    except FaceApiError as exc:
        return json_error(
            exc.message,
            status_code=exc.status_code,
            match=False,
            distance=None,
            threshold=MATCH_THRESHOLD,
        )
    except Exception:
        return json_error(
            "Erreur interne inattendue lors de la verification faciale.",
            status_code=500,
            match=False,
            distance=None,
            threshold=MATCH_THRESHOLD,
        )


if __name__ == "__main__":
    app.run(host=HOST, port=PORT, debug=DEBUG)
