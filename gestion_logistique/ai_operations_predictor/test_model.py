import pandas as pd
import numpy as np
from catboost import CatBoostRegressor
import os

def prepare_features(df_agg):
    df_agg = df_agg.sort_values(['ZoneOperation', 'TypeOperation', 'Date'])
    df_agg['day_of_week'] = df_agg['Date'].dt.dayofweek
    df_agg['month'] = df_agg['Date'].dt.month
    df_agg['week_of_year'] = df_agg['Date'].dt.isocalendar().week.astype(int)
    df_agg['is_weekend'] = df_agg['day_of_week'].isin([5, 6]).astype(int)
    
    df_agg['ops_lag_1'] = df_agg.groupby(['ZoneOperation', 'TypeOperation'])['num_operations'].shift(1)
    df_agg['ops_lag_7'] = df_agg.groupby(['ZoneOperation', 'TypeOperation'])['num_operations'].shift(7)
    df_agg['ops_rolling_mean_3'] = df_agg.groupby(['ZoneOperation', 'TypeOperation'])['num_operations'].transform(lambda x: x.shift(1).rolling(3).mean())
    return df_agg.fillna(0)

def predict_single(date_str, zone_name):
    base_dir = os.path.dirname(__file__)
    model_path = os.path.join(base_dir, 'model', 'model_final.cbm')
    csv_path = os.path.join(base_dir, '..', 'assets', 'data', 'operations_camions.csv')
    
    if not os.path.exists(model_path):
        print(f"Error: {model_path} not found. Run train_model.py first.")
        return

    model = CatBoostRegressor()
    model.load_model(model_path)
    
    df = pd.read_csv(csv_path, sep=';')
    df['DateDebut'] = pd.to_datetime(df['DateDebut'])
    df['DateFin'] = pd.to_datetime(df['DateFin'])
    df['duration_h'] = (df['DateFin'] - df['DateDebut']).dt.total_seconds() / 3600.0
    
    def count_ov(row, dataframe):
        mask = (dataframe['ZoneOperation'] == row['ZoneOperation']) & \
               (dataframe['DateDebut'] < row['DateFin']) & \
               (dataframe['DateFin'] > row['DateDebut'])
        return len(dataframe[mask]) - 1
    
    df['overlap_count'] = df.apply(lambda r: count_ov(r, df), axis=1)
    df['Date'] = df['DateDebut'].dt.date
    
    agg = df.groupby(['Date', 'ZoneOperation', 'TypeOperation']).agg(
        num_operations=('Immatriculation', 'count'),
        avg_priority=('Priorite', 'mean'),
        avg_duration=('duration_h', 'mean'),
        total_overlap=('overlap_count', 'sum')
    ).reset_index()
    agg['Date'] = pd.to_datetime(agg['Date'])
    
    target_date = pd.to_datetime(date_str)
    
    max_date = agg['Date'].max()
    min_date = agg['Date'].min()
    
    all_dates = pd.date_range(min_date, max(max_date, target_date), freq='D')
    zones = agg['ZoneOperation'].unique()
    types = agg['TypeOperation'].unique()
    idx = pd.MultiIndex.from_product([all_dates, zones, types], names=['Date', 'ZoneOperation', 'TypeOperation'])
    
    full_agg = pd.merge(pd.DataFrame(index=idx).reset_index(), agg, on=['Date', 'ZoneOperation', 'TypeOperation'], how='left').fillna(0)
    
    full_agg = prepare_features(full_agg)
    
    query = full_agg[(full_agg['Date'] == target_date) & (full_agg['ZoneOperation'] == zone_name)]
    
    if query.empty:
        print(f"Error: Zone '{zone_name}' not found in data.")
        return

    features = ['ZoneOperation', 'TypeOperation', 'day_of_week', 'month', 'week_of_year', 'is_weekend', 
                'avg_priority', 'avg_duration', 'total_overlap', 'ops_lag_1', 'ops_lag_7', 'ops_rolling_mean_3']
    
    preds = model.predict(query[features])
    total_est = np.sum(preds)
    
    actual_rows = agg[(agg['Date'] == target_date) & (agg['ZoneOperation'] == zone_name)]
    actual_count = actual_rows['num_operations'].sum() if not actual_rows.empty else None

    print("\n" + "="*35)
    print(f"Zone: {zone_name}")
    print(f"Date: {date_str} ({target_date.strftime('%A')})")
    print(f"Estimated operations: {total_est:.2f}")
    if actual_count is not None:
        print(f"Actual operations:    {actual_count}")
        error = abs(total_est - actual_count)
        print(f"Difference (Error):   {error:.2f}")
    else:
        print("Actual operations:    N/A (Future/Unknown date)")
    print("="*35)

if __name__ == "__main__":
    import sys
    quiet = "--quiet" in sys.argv
    args = [a for a in sys.argv if a != "--quiet"]
    
    d = args[1] if len(args) > 1 else "2025-03-25"
    z = args[2] if len(args) > 2 else "zone a"
    
    if quiet:
        import os
        import sys as s
        old_stdout = s.stdout
        from io import StringIO
        s.stdout = mystdout = StringIO()
        
        predict_single(d, z)
        
        s.stdout = old_stdout
        output = mystdout.getvalue()
        
        import re
        match = re.search(r"Estimated operations:\s+([\d\.-]+)", output)
        if match:
            print(match.group(1))
        else:
            print("0.00")
    else:
        predict_single(d, z)
