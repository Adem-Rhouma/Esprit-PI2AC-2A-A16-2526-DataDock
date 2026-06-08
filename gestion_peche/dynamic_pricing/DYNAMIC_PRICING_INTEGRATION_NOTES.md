# Notes d'integration Tarification Dynamique

## Perimetre

Cette implementation reste dans la page existante du `QStackedWidget` et reutilise uniquement :

- `ESPECES`
- `PECHES`
- `PECHESESPECES`
- `DP_PORT_CONFIG`
- `DP_PRICE_RECOMMENDATION_LOG`

Aucune table supplementaire n'est necessaire.

## Flux de donnees

`DpRepository::loadSpecies()` charge les especes depuis `ESPECES`.

`DpRepository::loadPorts()` charge les ports actifs et leurs coordonnees depuis `DP_PORT_CONFIG`.

`DpRepository::loadCatchContext()` assemble :

- le dernier prix local
- le prix moyen local
- la quantite totale
- le nombre de captures actives
- la derniere zone de peche
- le score de fraicheur a partir de `PECHES.DATEPECHE`

`MarineWeatherService` interroge Open-Meteo avec la latitude et la longitude du port selectionne.

`GfwIntelligenceService` interroge Global Fishing Watch autour du port selectionne. Le jeton est lu depuis :

- `QSettings` cle `dynamic_pricing/gfw_token`
- secours `QSettings` cle `DynamicPricing/gfwToken`
- secours variable d'environnement `GFW_TOKEN`

Si le jeton manque ou si l'API echoue, la page reste utilisable et indique explicitement l'etat de secours.

## Formule

`recommande = base + fraicheur + disponibilite + meteo - risque + activite_externe + prime_zone`

Poids :

- fraicheur `16%`
- disponibilite `14%`
- meteo `8%`
- risque `12%`
- activite externe `10%`
- zone `4%`

Le resultat est borne entre `78%` et `142%` du prix de base local.

## Persistance

Seule la recommandation finale est enregistree dans `DP_PRICE_RECOMMENDATION_LOG` :

- prix de base
- prix recommande
- score meteo
- score risque
- score fraicheur
- explication
- source API

Aucune extension de schema n'est requise.
