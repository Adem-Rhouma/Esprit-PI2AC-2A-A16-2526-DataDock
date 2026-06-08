import json
import os
import re

class FishEnricher:
    def __init__(self, dataset_path):
        self.dataset_path = dataset_path
        self.data = self._load_data()

    def _load_data(self):
        if os.path.exists(self.dataset_path):
            try:
                with open(self.dataset_path, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except Exception:
                return []
        return []

    def _save_data(self):
        with open(self.dataset_path, 'w', encoding='utf-8') as f:
            json.dump(self.data, f, indent=2, ensure_ascii=False)

    def get_fish_info(self, species):
        # Check local cache first
        species_lower = species.lower()
        for item in self.data:
            if item['species'].lower() == species_lower:
                return item
        
        # Strictly local fallback (no scraping as per user request)
        print(f"Species {species} not found in dataset. Using local heuristic fallback.")
        return self._generate_fallback_info(species)

    def _generate_fallback_info(self, species):
        """Strictly local rules based on common species knowledge"""
        species_lower = species.lower()
        
        # Default values
        sensitivity = "Medium"
        opt_temp = [-18.0, -15.0]
        speed = 0.04

        # Heuristic rules based on common fish types
        if any(s in species_lower for s in ["thon", "tuna", "espadon", "swordfish"]):
            sensitivity = "High"
            opt_temp = [-25.0, -20.0]
            speed = 0.06
        elif any(s in species_lower for s in ["sardine", "maquereau", "anchovy", "hareng"]):
            sensitivity = "High"
            opt_temp = [-5.0, -2.0]
            speed = 0.08
        elif any(s in species_lower for s in ["poulpe", "octopus", "calamar", "squid"]):
            sensitivity = "Medium"
            opt_temp = [-22.0, -18.0]
            speed = 0.05
        elif any(s in species_lower for s in ["crevette", "shrimp", "homard", "langouste"]):
            sensitivity = "High"
            opt_temp = [-18.0, -15.0]
            speed = 0.07
        elif any(s in species_lower for s in ["merlu", "cabillaud", "morue", "cod"]):
            sensitivity = "Medium"
            opt_temp = [-18.0, -15.0]
            speed = 0.04

        return {
            "species": species,
            "sensitivity": sensitivity,
            "degradation_speed": speed,
            "optimal_temp_range": opt_temp,
            "humidity_preference": 85,
            "source": "local_heuristics"
        }

if __name__ == "__main__":
    # Test
    enricher = FishEnricher("data/fish_dataset.json")
    print(enricher.get_fish_info("Thon"))
    print(enricher.get_fish_info("Crevette")) 
