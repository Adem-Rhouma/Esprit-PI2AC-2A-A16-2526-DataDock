import pandas as pd
import numpy as np
from catboost import CatBoostRegressor
from sklearn.model_selection import train_test_split
import os

def prepare_and_train(csv_path):
    print(f"Loading {csv_path}...")
    df = pd.read_csv(csv_path, sep=';')
    df['Date'] = pd.to_datetime(df['DateDebut']).dt.date
    
    # Aggregation
    agg = df.groupby(['Date', 'ZoneOperation', 'TypeOperation']).agg(
        num_operations=('Immatriculation', 'count'),
        avg_priority=('Priorite', 'mean')
    ).reset_index()
    agg['Date'] = pd.to_datetime(agg['Date'])
    
    # Re-indexing to capture "zero activity" days
    all_dates = pd.date_range(agg['Date'].min(), agg['Date'].max(), freq='D')
    zones = agg['ZoneOperation'].unique()
    types = agg['TypeOperation'].unique()
    idx = pd.MultiIndex.from_product([all_dates, zones, types], names=['Date', 'ZoneOperation', 'TypeOperation'])
    agg = pd.merge(pd.DataFrame(index=idx).reset_index(), agg, on=['Date', 'ZoneOperation', 'TypeOperation'], how='left').fillna(0)
    
    # ── Feature Engineering ──
    def add_cyclical_features(df):
        df['day_of_week'] = df['Date'].dt.dayofweek
        df['month'] = df['Date'].dt.month
        df['week'] = df['Date'].dt.isocalendar().week.astype(int)
        
        # Periodic encodings for time (Monday same as Sunday, etc)
        df['weekday_sin'] = np.sin(2 * np.pi * df['day_of_week'] / 7)
        df['weekday_cos'] = np.cos(2 * np.pi * df['day_of_week'] / 7)
        df['month_sin'] = np.sin(2 * np.pi * df['month'] / 12)
        df['month_cos'] = np.cos(2 * np.pi * df['month'] / 12)
        
        # Lag features
        df = df.sort_values(['ZoneOperation', 'TypeOperation', 'Date'])
        groups = df.groupby(['ZoneOperation', 'TypeOperation'])
        df['ops_lag_1'] = groups['num_operations'].shift(1)
        df['ops_lag_7'] = groups['num_operations'].shift(7)
        df['ops_rolling_3'] = groups['num_operations'].transform(lambda x: x.shift(1).rolling(3).mean())
        return df.fillna(0)

    agg = add_cyclical_features(agg)
    
    features = ['ZoneOperation', 'TypeOperation', 'weekday_sin', 'weekday_cos', 'month_sin', 'month_cos', 
                'avg_priority', 'ops_lag_1', 'ops_lag_7', 'ops_rolling_3']
    cat_features = ['ZoneOperation', 'TypeOperation']
    
    X = agg[features]
    y = agg['num_operations']
    
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.15, random_state=42)
    
    # Train with higher sensitivity to features
    model = CatBoostRegressor(
        iterations=1000,
        learning_rate=0.05,
        depth=8,
        l2_leaf_reg=3,
        random_seed=42,
        verbose=200
    )
    
    print("\nTraining high-fidelity model...")
    model.fit(X_train, y_train, cat_features=cat_features, eval_set=(X_test, y_test), early_stopping_rounds=50)
    
    model_dir = os.path.dirname(__file__)
    model.save_model(os.path.join(model_dir, 'model_final.cbm'))
    print(f"\nModel re-trained and saved at {os.path.join(model_dir, 'model_final.cbm')}")

if __name__ == "__main__":
    base_dir = os.path.dirname(__file__)
    csv_path = os.path.join(base_dir, '..', '..', 'assets', 'data', 'operations_camions.csv')
    prepare_and_train(csv_path)
