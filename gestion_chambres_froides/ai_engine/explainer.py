import json
import os
import urllib.request
import urllib.error

class AIExplainer:
    def __init__(self, ollama_url="http://localhost:11434/api/generate", model="qwen2.5:0.5b"):
        self.ollama_url = ollama_url
        self.model = model

    def explain_solution(self, solution):
        """
        Génère une explication experte via Ollama (qwen2.5:0.5b).
        Tombe en repli sur le template déterministe si Ollama est indisponible.
        """
        try:
            prompt = f"""[SYSTEME: EXPERT LOGISTIQUE ET SÉCURITÉ ALIMENTAIRE]
En tant qu'expert en gestion de la chaîne du froid pour l'industrie halieutique, analyse ce plan d'entreposage.
Données techniques: {json.dumps(solution, ensure_ascii=False)}

Ta mission: Fournir une analyse stratégique approfondie pour un directeur de port.
Tu DOIS structurer ta réponse exactement avec ces sections (emojis inclus):

📍 RÉSUMÉ LOGISTIQUE: (Analyse des flux. Ex: "Traitement de X kg sur Y zones, avec une utilisation optimale de la zone CF-XX pour minimiser les ruptures de charge.")
💡 JUSTIFICATION STRATÉGIQUE: (Pourquoi cette configuration ? Parle d'inertie thermique, d'affinités de température, et de réduction de la consommation énergétique liée aux ouvertures de portes.)
⚠️ POINTS DE VIGILANCE: (Quels sont les goulots d'étranglement ou risques de saturation ? Mentionne les seuils de température critiques.)
🚀 CONSEILS D'EXPERT: (Recommandations FIFO, gestion de la rotation, maintenance préventive des sondes.)

Ton: Technique, décisionnel, précis. Pas de généralités.
"""
            
            data = {
                "model": self.model,
                "prompt": prompt,
                "stream": False,
                "options": {
                    "temperature": 0.2, # Lower temperature for more consistent professional output
                    "num_ctx": 4096
                }
            }
            
            req = urllib.request.Request(
                self.ollama_url, 
                data=json.dumps(data).encode('utf-8'),
                headers={'Content-Type': 'application/json'},
                method='POST'
            )
            
            with urllib.request.urlopen(req, timeout=12) as response:
                result = json.loads(response.read().decode('utf-8'))
                explanation = result.get('response', "")
                if explanation:
                    return explanation
        except Exception as e:
            print(f"Ollama Error (Model {self.model}): {e}. Falling back to template.")
            
        return self._smart_template_explanation(solution)

    def _smart_template_explanation(self, solution):
        """Générateur d'expertise déterministe enrichie (Repli de Haute Qualité)"""
        allocs = solution.get('allocations', [])
        num_batches = len(allocs)
        total_kg = solution.get('total_kg', sum(a.get('quantity', 0) for a in allocs))
        score = solution.get('score', 0)
        
        if num_batches == 0:
            return "📍 RÉSUMÉ LOGISTIQUE\nAucun flux de stock n'a pu être alloué selon les contraintes de sécurité actuelles."

        avg_diff = sum(a.get('temp_diff', 0) for a in allocs) / num_batches
        predictions = solution.get('predictions', {})
        risk = predictions.get('spoilage_risk', 0.05)
        shelf_life = predictions.get('estimated_shelf_life_days', 15)
        
        explanation = f"📍 RÉSUMÉ LOGISTIQUE\n"
        explanation += f"Optimisation de **{total_kg}kg** de produits halieutiques. Le plan assure une répartition sur {num_batches} points de contrôle CCP. "
        explanation += f"L'indice de performance logistique s'établit à **{score}/100**, validant la faisabilité opérationnelle.\n\n"
        
        explanation += "💡 JUSTIFICATION STRATÉGIQUE\n"
        if avg_diff < 1.0:
            explanation += "Ce scénario maximise l'**inertie thermique** du stock. L'écart moyen de température est quasi nul, ce qui prévient le choc thermique post-déchargement et optimise la dépense énergétique du groupe froid.\n\n"
        else:
            explanation += "Configuration en **mode saturation maîtrisée**. Les lots sont affectés pour garantir une circulation d'air (convection) minimale entre les palettes, limitant ainsi la création de points chauds dans les zones denses.\n\n"
            
        explanation += "⚠️ POINTS DE VIGILANCE\n"
        explanation += f"Le risque de dégradation enzymatique (Indice Spoilage) est de {round(risk * 100, 1)}%. "
        if shelf_life < 5:
            explanation += "ALERTE : Durée de conservation critique (< 5j). Nécessite une évacuation immédiate du stock.\n\n"
        else:
            explanation += f"Stabilité biologique confirmée pour une période de {shelf_life} jours sous réserve de maintien de la chaîne du froid.\n\n"
            
        explanation += "🚀 CONSEILS D'EXPERT\n"
        explanation += "1. Appliquer strictement la règle **FIFO** (Oldest Stock out first).\n"
        explanation += "2. Minimiser les temps d'ouverture des portes lors du chargement.\n"
        explanation += "3. Inspection préventive des évaporateurs en zone de forte charge.\n"
        
        return explanation

if __name__ == "__main__":
    ex = AIExplainer()
    # Test simulation
    test_sol = {
        "solution_id": "TEST-01",
        "allocations": [{"species": "Thon", "quantity": 100, "cold_room": "CF01", "temp_diff": 0.5}],
        "predictions": {"spoilage_risk": 0.02, "estimated_shelf_life_days": 20}
    }
    print(ex.explain_solution(test_sol))
