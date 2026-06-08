import sys
import json
import numpy as np
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.naive_bayes import MultinomialNB
from sklearn.pipeline import Pipeline

TRAINING_DATA = [
    ("bruit métallique moteur", "Vérification des bielles et de la lubrification requise."),
    ("vibration suspecte coque", "Inspection de l'arbre d'hélice et de l'équilibrage."),
    ("fumée noire échappement", "Analyse de l'injection et nettoyage des injecteurs."),
    ("perte de puissance", "Vérification des filtres à carburant et du turbocompresseur."),
    ("fuite huile", "Changement des joints de culasse ou durites d'huile."),
    ("son aigu sifflement", "Contrôle de la courroie de l'alternateur ou du turbo."),
    ("direction dure", "Vérification du système hydraulique de gouvernail."),
    ("alarme température", "Nettoyage du circuit de refroidissement et pompe à eau."),
    ("consommation excessive fuel", "Calibrage moteur et vérification de la carène (anti-fouling)."),
    ("voyage normal calme", "Maintenance standard préventive uniquement."),
    ("moteur brûle feu incendie", "URGENT: Incendie moteur détecté. Remplacement immédiat requis."),
    ("moteur ne fonctionne plus arrêt panne", "Panne totale moteur. Remorquage et révision complète nécessaires."),
    ("casse moteur explosion", "Dégâts structurels moteur. Remplacement du bloc moteur recommandé."),
    ("eau dans la cale fuite", "Axe d'étanchéité ou coque percée. Mise en cale sèche urgente.")
]

def train_nlp_model():
    texts = [item[0] for item in TRAINING_DATA]
    labels = [item[1] for item in TRAINING_DATA]
    model = Pipeline([
        ('tfidf', TfidfVectorizer(ngram_range=(1, 2))),
        ('clf', MultinomialNB())
    ])
    model.fit(texts, labels)
    return model

def main():
    try:
        input_data = json.load(sys.stdin)
        logs = input_data.get('logs', [])
        
        if not logs:
            print(json.dumps({"error": "No logs available for NLP analysis"}))
            return

        nlp_model = train_nlp_model()
        suggestions = []
        
        descriptions = [log.get('description') for log in logs if log.get('description')]
        
        if descriptions:
            latest_desc = descriptions[-1].lower()
            
            # 1. NLP Model Prediction
            raw_suggestion = nlp_model.predict([latest_desc])[0]
            probs = nlp_model.predict_proba([latest_desc])[0]
            if np.max(probs) > 0.05: # Lower threshold
                suggestions.append(raw_suggestion)
            
            # 2. Critical Keyword Fallback (Direct Match)
            critical_mappings = {
                "brule": "URGENT: Incendie moteur détecté. Remplacement immédiat requis.",
                "feu": "URGENT: Incendie moteur détecté. Remplacement immédiat requis.",
                "panne": "Panne totale moteur. Remorquage et révision complète nécessaires.",
                "ne fonctionne plus": "Panne totale moteur. Remorquage et révision complète nécessaires.",
                "explosion": "Dégâts structurels moteur. Remplacement du bloc moteur recommandé."
            }
            for key, val in critical_mappings.items():
                if key in latest_desc:
                    suggestions.append(val)

        if logs:
            last_log = logs[-1]
            if float(last_log.get('fuel', 0)) > 500 and "carène" not in str(suggestions):
                suggestions.append("Consommation suspecte pour ce trajet. Vérifier l'état de la coque.")

        result = {
            "suggestions": list(set(suggestions))
        }

        print(json.dumps(result))

    except Exception as e:
        print(json.dumps({"error": str(e)}))

if __name__ == "__main__":
    main()
