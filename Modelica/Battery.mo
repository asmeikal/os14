class Battery
  extends Appliance;
  parameter Real capacity(unit = "kWh");
  parameter Real chargeDissipation;
  parameter Real dischargeDissipation;
  Real charge(unit = "kW", min = capacity * 0.2, max = capacity);
  Real chargeRate_onGrid(unit = "kWh");
  Real chargeRate_onLoad(unit = "kWh");
equation
  chargeRate_onGrid = if chargeRate >= 0.0 then chargeRate else chargeRate * dischargeDissipation;
  chargeRate_onLoad = if chargeRate >= 0.0 then chargeRate * chargeDissipation else chargeRate;
  der(charge) = chargeRate_onLoad;
end Battery;