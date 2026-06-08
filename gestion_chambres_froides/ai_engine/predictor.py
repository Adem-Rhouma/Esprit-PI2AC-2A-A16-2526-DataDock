import os
import joblib
import numpy as np

class FreshnessPredictor:
    def __init__(self, model_path=None):
        if model_path is None:
            model_path = os.path.join(os.path.dirname(__file__), 'shelf_life_model.joblib')
        
        self.model = None
        if os.path.exists(model_path):
            try:
                self.model = joblib.load(model_path)
            except Exception as e:
                print(f"Error loading ML model: {e}")

    def _get_species_index(self, species_name):
        s = species_name.lower()
        if "thon" in s: return 0
        if "saumon" in s: return 1
        if "crevette" in s: return 2
        if "sardine" in s: return 3
        if "loup" in s: return 4
        if "daurade" in s: return 5
        return 6

    def predict(self, species_info, current_temp, days_forecast=30):
        """
        Predicts freshness decay over time using an ML model (RandomForest).
        Falls back to a hybrid formula if the model is missing.
        """
        species_name = species_info.get("species", "Autre")
        target_temp = species_info.get("optimal_temp_range", [-18, -15])[0]
        qty = species_info.get("quantity", 100)
        
        species_idx = self._get_species_index(species_name)
        
        spoilage_risk = 0.0
        shelf_life = 30.0
        
        if self.model:
            # Features: [species, current_temp, target_temp, quantity]
            features = [[species_idx, current_temp, target_temp, qty]]
            prediction = self.model.predict(features)[0]
            spoilage_risk = float(prediction[0])
            shelf_life = float(prediction[1])
        else:
            # Fallback deterministic
            base_rate = species_info.get("degradation_speed", 0.04)
            delta = max(0, current_temp - target_temp)
            temp_factor = 1.0 + (delta * 0.1)
            effective_rate = base_rate * temp_factor
            spoilage_risk = 1.0 - np.exp(-effective_rate * 7)
            shelf_life = np.log(0.5) / -effective_rate if effective_rate > 0 else 30
            
        # Generate decay curve based on predicted shelf life
        # At shelf_life, freshness is 50%
        # Freshness(t) = 100 * exp(-k * t), where k = -log(0.5)/shelf_life
        k = -np.log(0.5) / shelf_life if shelf_life > 0 else 0.04
        
        curve = []
        for d in range(days_forecast + 1):
            freshness = 100.0 * np.exp(-k * d)
            curve.append({
                "day": d,
                "freshness": round(float(freshness), 2)
            })

        analysis = []
        if current_temp > target_temp:
            analysis.append(f"Observation : Stockage à {current_temp}°C (Cible: {target_temp}°C).")
            analysis.append(f"Alerte : Écart thermique de {round(current_temp - target_temp, 1)}°C impactant la structure cellulaire.")
        else:
            analysis.append(f"Observation : Température de conservation ({current_temp}°C) optimale.")

        # Predicted Milestones
        three_day = curve[3]["freshness"] if len(curve) > 3 else 50
        seven_day = curve[7]["freshness"] if len(curve) > 7 else 0
        
        analysis.append(f"Prédiction ML (7j) : Indice de fraîcheur estimé à {round(seven_day, 1)}%.")
        
        if shelf_life > 14:
            analysis.append("Verdict : Longue conservation possible. Qualité Premium.")
        elif shelf_life > 7:
            analysis.append("Verdict : Conservation stable. Rotation standard recommandée.")
        else:
            analysis.append("Verdict : Rotation PRIORITAIRE requise. Risque de dégradation rapide.")

        return {
            "freshness_curve": curve,
            "spoilage_risk": round(spoilage_risk, 4),
            "estimated_shelf_life_days": round(shelf_life, 1),
            "detailed_analysis": "\n".join(analysis),
            "model_active": self.model is not None
        }

if __name__ == "__main__":
    p = FreshnessPredictor()
    print(p.predict({"degradation_speed": 0.05}, -18))
