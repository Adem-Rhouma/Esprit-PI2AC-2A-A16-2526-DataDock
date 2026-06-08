import pandas as pd
import numpy as np
from catboost import CatBoostRegressor
import os
import json
from datetime import datetime, timedelta

def predict_range():
    base_dir = os.path.dirname(__file__)
    model_path = os.path.join(base_dir, 'model', 'model_final.cbm')
    csv_path = os.path.join(base_dir, '..', 'assets', 'data', 'operations_camions.csv')
    
    print(f"DEBUG: Model path: {model_path}")
    print(f"DEBUG: CSV path: {csv_path}")

    if not os.path.exists(model_path):
        return {"error": f"Model file missing at {model_path}"}
    if not os.path.exists(csv_path):
        return {"error": f"Data file missing at {csv_path}"}

    model = CatBoostRegressor()
    model.load_model(model_path)
    
    df = pd.read_csv(csv_path, sep=';')
    if df.empty:
        return {"error": "No historical data found in operations_camions.csv"}
    
    df['Date'] = pd.to_datetime(df['DateDebut']).dt.date
    if df['Date'].isnull().all():
        return {"error": "Invalid date format in CSV"}
    
    agg = df.groupby(['Date', 'ZoneOperation', 'TypeOperation']).agg(
        num_operations=('Immatriculation', 'count'),
        avg_priority=('Priorite', 'mean')
    ).reset_index()
    agg['Date'] = pd.to_datetime(agg['Date'])
    
    # 1. Historical averages per Zone/Type
    zone_stats = agg.groupby(['ZoneOperation', 'TypeOperation'])['avg_priority'].mean().to_dict()
    global_avg_prio = agg['avg_priority'].mean()
    
    # Range setup
    start_date = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    end_date = start_date + timedelta(days=90)
    
    all_dates = pd.date_range(agg['Date'].min(), end_date, freq='D')
    zones = agg['ZoneOperation'].unique()
    types = agg['TypeOperation'].unique()
    idx = pd.MultiIndex.from_product([all_dates, zones, types], names=['Date', 'ZoneOperation', 'TypeOperation'])
    
    full_agg = pd.merge(pd.DataFrame(index=idx).reset_index(), agg, on=['Date', 'ZoneOperation', 'TypeOperation'], how='left').fillna(0)
    full_agg = full_agg.sort_values(['Date', 'ZoneOperation', 'TypeOperation'])
    full_agg.set_index(['Date', 'ZoneOperation', 'TypeOperation'], inplace=True)

    future_dates = pd.date_range(start_date, end_date, freq='D')
    
    for d in future_dates:
        rows = []
        indices = []
        for z in zones:
            for t in types:
                # ── Features ──
                # Lags
                l1 = full_agg.loc[(d - timedelta(days=1), z, t), 'num_operations'] if (d - timedelta(days=1), z, t) in full_agg.index else 0
                l7 = full_agg.loc[(d - timedelta(days=7), z, t), 'num_operations'] if (d - timedelta(days=7), z, t) in full_agg.index else 0
                
                rm3_list = [full_agg.loc[(d - timedelta(days=i), z, t), 'num_operations'] for i in [1, 2, 3] if (d - timedelta(days=i), z, t) in full_agg.index]
                rm3 = np.mean(rm3_list) if rm3_list else 0
                
                # Cyclical features
                wd = d.dayofweek
                mon = d.month
                
                rows.append([
                    z, t, 
                    np.sin(2 * np.pi * wd / 7), np.cos(2 * np.pi * wd / 7),
                    np.sin(2 * np.pi * mon / 12), np.cos(2 * np.pi * mon / 12),
                    zone_stats.get((z, t), global_avg_prio),
                    l1, l7, rm3
                ])
                indices.append((d, z, t))

        if rows:
            preds = model.predict(rows)
            for i, p in enumerate(preds):
                # Apply a slight sensitivity factor to highlight variance into the future
                # into the recursive loop, to avoid too aggressive regression to mean
                full_agg.at[indices[i], 'num_operations'] = max(0, float(p))

    # Final result and grouping
    full_agg.reset_index(inplace=True)
    result = full_agg[full_agg['Date'] >= start_date].groupby(['ZoneOperation', 'Date'])['num_operations'].sum().reset_index()
    
    output = {}
    for zone in zones:
        zone_data = result[result['ZoneOperation'] == zone]
        output[zone] = [
            {"date": d.strftime('%Y-%m-%d'), "val": round(float(v), 2)}
            for d, v in zip(zone_data['Date'], zone_data['num_operations'])
        ]
    return output

if __name__ == "__main__":
    try:
        data = predict_range()
        output_path = os.path.join(os.path.dirname(__file__), 'predictions_3months.json')
        with open(output_path, 'w') as f:
            json.dump(data, f, indent=4)
    except Exception as e:
        error_path = os.path.join(os.path.dirname(__file__), 'predictions_3months.json')
        with open(error_path, 'w') as f:
            json.dump({"error": str(e)}, f, indent=4)
