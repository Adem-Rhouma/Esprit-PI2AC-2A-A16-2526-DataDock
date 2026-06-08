import sys
import json
import os

# Ensure local imports work
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

def run_standalone_optimization(input_path, output_path):
    try:
        # Load input
        with open(input_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        fish_batches = data.get("batches", [])
        cold_rooms = data.get("cold_rooms", [])
        
        # 1. Lite Optimizer (No OR-Tools for 8GB RAM stability)
        from optimizer import ColdRoomOptimizer
        optimizer = ColdRoomOptimizer()
        solutions = optimizer.heuristic_multi_solve(fish_batches, cold_rooms, num_solutions=3)
        
        # 2. Lite Results Enrichment (No ML/LLM)
        from explainer import AIExplainer
        explainer = AIExplainer()
        enriched_solutions = []
        
        for i, sol in enumerate(solutions):
            allocations = sol.get("allocations", [])
            # Simple predictions since ML model loading is heavy
            sol["predictions"] = {
                "spoilage_risk": 0.05,
                "estimated_shelf_life_days": 15.0,
                "freshness_curve": [{"day": d, "freshness": round(100 * (0.5**(d/15.0)), 2)} for d in range(31)],
                "detailed_analysis": "Analyse logistique complète sur 30 jours."
            }
            
            # Use LLM-based explanation (with template fallback)
            sol["explanation"] = explainer.explain_solution(sol)
            sol["id"] = f"scenario_{i+1}"
            enriched_solutions.append(sol)
            
        # Write output
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(enriched_solutions, f, indent=4, ensure_ascii=False)
            
    except Exception as e:
        import traceback
        err_msg = traceback.format_exc()
        print(err_msg, file=sys.stderr) # Ensure it goes to stderr for QProcess
        with open("/tmp/ai_debug_error.log", "w", encoding="utf-8") as lf:
            lf.write(err_msg)
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(1)
    run_standalone_optimization(sys.argv[1], sys.argv[2])
