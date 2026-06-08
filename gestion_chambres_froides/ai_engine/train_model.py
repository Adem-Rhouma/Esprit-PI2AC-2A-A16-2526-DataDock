import numpy as np
import pandas as pd
from sklearn.ensemble import RandomForestRegressor
import joblib
import os

def generate_data(n_samples=1000):
    np.random.seed(42)
    # 0: Thon, 1: Saumon, 2: Crevette, 3: Sardine, 4: Loup, 5: Daurade, 6: Autre
    species = np.random.randint(0, 7, n_samples)
    current_temp = np.random.uniform(-25, 10, n_samples)
    target_temp = np.random.uniform(-22, -15, n_samples)
    quantity = np.random.uniform(10, 5000, n_samples)
    
    # Base degradation rates (lower is better)
    base_rates = {0: 0.03, 1: 0.05, 2: 0.08, 3: 0.06, 4: 0.04, 5: 0.04, 6: 0.07}
    
    risk = []
    shelf_life = []
    
    for i in range(n_samples):
        br = base_rates[species[i]]
        # Temp penalty: exponentially worse as current_temp exceeds target_temp
        delta = max(0, current_temp[i] - target_temp[i])
        temp_penalty = 1.0 + (delta * 0.1) + (delta**2 * 0.01)
        
        effective_rate = br * temp_penalty
        
        # Risk after 7 days: 1 - exp(-rate * 7)
        r = 1.0 - np.exp(-effective_rate * 7)
        r = np.clip(r + np.random.normal(0, 0.01), 0, 1)
        
        # Shelf life: until freshness hits 50%
        sl = np.log(0.5) / -effective_rate
        sl = np.clip(sl + np.random.normal(0, 0.5), 1, 60)
        
        risk.append(r)
        shelf_life.append(sl)
        
    df = pd.DataFrame({
        'species': species,
        'current_temp': current_temp,
        'target_temp': target_temp,
        'quantity': quantity,
        'risk': risk,
        'shelf_life': shelf_life
    })
    return df

def train():
    print("Generating synthetic data...")
    df = generate_data()
    
    X = df[['species', 'current_temp', 'target_temp', 'quantity']]
    y = df[['risk', 'shelf_life']]
    
    print("Training RandomForestRegressor...")
    model = RandomForestRegressor(n_estimators=100, random_state=42)
    model.fit(X, y)
    
    model_dir = os.path.dirname(__file__)
    model_path = os.path.join(model_dir, 'shelf_life_model.joblib')
    
    print(f"Saving model to {model_path}...")
    joblib.dump(model, model_path)
    print("Training complete.")

if __name__ == "__main__":
    train()
