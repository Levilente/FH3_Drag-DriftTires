# Changelog

## 1.0.0

First release of **Drag and Drift Tires for Forza Horizon 3**, on the [FH3 Mod Platform](https://github.com/Levilente/FH3_ModPlatform).

- **Garage > Upgrades > Tires and Rims > Tire Compound** gets two new choices on every car: **Drag Tire Compound** and **Drift Tire Compound** (10 000 CR each by default).
- The drag compound is a new compound made from the Race compound with **170 % of its straight-line grip and 55 % of its cornering grip**, on the car's own **Race** tire models (the game's own `19_Drag` is weaker than Race on a cold tire and its `tire_drag` model shows a car with no tires, so neither is used); the drift compound is the game's own hidden `18_Drift` compound on the car's own **Sport** tire models.
- Pure data: the mod changes the game's database through the platform (`add_sql`), nothing in the game's files is touched.
- No settings file: the values are fixed in the mod.
- Needs FH3 Mod Platform 1.2.0 or newer. The platform's update check knows this repository.

## 1.0.1

- **Drift Tire Compound** is a new low-grip compound made from the Sport compound (55 % acceleration, 65 % cornering, 80 % braking grip), instead of the game's own `18_Drift`, which has more acceleration grip than Race and made a powerful car almost unable to break traction.

## 1.0.2

- Drift Tire Compound grip raised, it was too slippery to pull away: 80 % acceleration, 80 % cornering, 90 % braking grip of the Sport compound.
