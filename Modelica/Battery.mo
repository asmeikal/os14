class Battery
  parameter Real maxChargeRate(unit = "kW");
  parameter Real minChargeRate(unit = "kW");
  parameter Real startingCharge(unit = "kWh");
  parameter Real chargeDissipation;
  parameter Real dischargeDissipation;
  parameter Real capacity(unit = "kWh");
  Real chargeRate(unit = "kW", min = minChargeRate, max = maxChargeRate);
  Real charge(unit = "kWh", min = capacity * 0.2, max = capacity, start = startingCharge);
  Real chargeRate_toGrid(unit = "kWh");
  Real chargeRate_toLoad(unit = "kWh");
equation
  chargeRate_toGrid = if chargeRate >= 0 then chargeRate else chargeRate * dischargeDissipation;
  chargeRate_toLoad = if chargeRate >= 0 then chargeDissipation * chargeRate else chargeRate;
  der(charge) = chargeRate_toLoad / 60;
end Battery;