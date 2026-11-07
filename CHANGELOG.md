# Changelog

## 1.0.0

First release of **Drag and Drift Tires for Forza Horizon 3**, on the [FH3 Mod Platform](https://github.com/Levilente/FH3_ModPlatform).

- **Garage > Upgrades > Tires and Rims > Tire Compound** gets two new choices on every car: **Drag Tire Compound** and **Drift Tire Compound** (10 000 CR each by default).
- The drag compound is the game's own hidden `19_Drag` compound on the car's own **Race** tire models (the game's `tire_drag` model shows a car with no tires, so it isn't used); the drift compound is the game's own hidden `18_Drift` compound on the car's own **Sport** tire models.
- Pure data: the mod changes the game's database through the platform (`add_sql`), nothing in the game's files is touched.
