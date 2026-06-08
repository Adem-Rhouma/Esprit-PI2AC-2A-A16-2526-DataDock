import sys
import json
import numpy as np
from sklearn.ensemble import RandomForestRegressor
from datetime import datetime, timedelta

def main():
    try:
        input_data = json.load(sys.stdin)
        logs = input_data.get('logs', [])
        
        if not logs:
            print(json.dumps({"error": "No logs available for prediction"}))
            return

        X = []
        y = []
        current_date = datetime.now()
        
        for log in logs:
            dist = float(log.get('distance', 0))
            fuel = float(log.get('fuel', 0))
            cargo = float(log.get('cargo', 0))
            X.append([dist, fuel, cargo])
            y.append(max(0, 100 - (dist / 10)))

        X = np.array(X)
        y = np.array(y)
        model_rf = RandomForestRegressor(n_estimators=10)
        model_rf.fit(X, y)

        avg_features = np.mean(X[-3:], axis=0).reshape(1, -1) if len(X) >= 3 else np.mean(X, axis=0).reshape(1, -1)
        predicted_readiness = model_rf.predict(avg_features)[0]

        # --- Link Symptom Analysis to Score ---
        # Penalize readiness if red-flag keywords are found in the latest description
        symptom_penalty = 0
        latest_desc = ""
        descriptions = [log.get('description', '') for log in logs if log.get('description')]
        if descriptions:
            latest_desc = descriptions[-1].lower()
            red_flags = {
                "bruit": 20, "vibration": 15, "fumée": 25, 
                "fuite": 20, "perte": 15, "sifflement": 10,
                "dur": 10, "alarme": 30, "température": 25,
                "brule": 90, "feu": 90, "panne": 80, "fonctionne plus": 85,
                "casse": 80, "eau": 40
            }
            for flag, penalty in red_flags.items():
                if flag in latest_desc:
                    symptom_penalty = max(symptom_penalty, penalty)
        
        # --- Link Fuel Efficiency to Score ---
        fuel_penalty = 0
        if logs:
            last_log = logs[-1]
            last_dist = float(last_log.get('distance', 0))
            last_fuel = float(last_log.get('fuel', 0))
            if last_dist > 0:
                ratio = last_fuel / last_dist
                if ratio > 10: # Abnormal: more than 10L per nm
                    fuel_penalty = min(60, (ratio - 10) * 5)
        
        # Apply combined penalties
        predicted_readiness = max(0, predicted_readiness - symptom_penalty - fuel_penalty)

        days_to_maint = int(predicted_readiness / 5)
        # Advance maintenance if symptoms/fuel are severe
        if symptom_penalty > 50 or fuel_penalty > 40: 
            days_to_maint = 0 # Immediate maintenance
        elif symptom_penalty > 20:
            days_to_maint = min(days_to_maint, 3)
        
        next_maint = (current_date + timedelta(days=days_to_maint)).strftime('%Y-%m-%d')

        chart_data = []
        for d in range(10):
            prob = max(0, min(100, predicted_readiness - d * 5))
            chart_data.append({"day": (current_date + timedelta(days=d)).strftime('%d/%m'), "value": prob})

        result = {
            "predicted_readiness": round(predicted_readiness, 1),
            "next_maintenance_date": next_maint,
            "days_to_maintenance": days_to_maint,
            "chart_data": chart_data
        }

        print(json.dumps(result))

    except Exception as e:
        print(json.dumps({"error": str(e)}))

if __name__ == "__main__":
    main()
