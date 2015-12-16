model HouseLUT
  Real energyConsumption(unit = "kW");
  HouseData HouseSim;
  Battery mainBattery(capacity = 4, minChargeRate = -2, maxChargeRate = 2, startingCharge = 2, chargeDissipation = 0.98, dischargeDissipation = 0.82);
  PHEV myCar(capacity = 16, maxChargeRate = 13, chargeDissipation = 0.876, dischargeDissipation = 0.0, startingCharge = 0);

  class Battery
    parameter Real maxChargeRate(unit = "kW");
    parameter Real minChargeRate(unit = "kW");
    parameter Real startingCharge(unit = "kWh");
    parameter Real chargeDissipation;
    parameter Real dischargeDissipation;
    parameter Real capacity(unit = "kWh");
    Real chargeRate(unit = "kW", min = minChargeRate, max = maxChargeRate);
    Real charge(unit = "kWh", min = capacity * 0.19, max = capacity * 1.01, start = startingCharge);
    Real chargeRate_toGrid(unit = "kWh");
    Real chargeRate_toLoad(unit = "kWh");
  equation
    chargeRate_toGrid = if chargeRate >= 0 then chargeRate else chargeRate * dischargeDissipation;
    chargeRate_toLoad = if chargeRate >= 0 then chargeDissipation * chargeRate else chargeRate;
    der(charge) = chargeRate_toLoad / 60;
  end Battery;

  class PHEV
    parameter Real maxChargeRate(unit = "kW");
    parameter Real minChargeRate(unit = "kW") = 0;
    parameter Real startingCharge(unit = "kWh");
    parameter Real chargeDissipation;
    parameter Real dischargeDissipation;
    parameter Real capacity(unit = "kWh");
    Real chargeRate(unit = "kW", min = minChargeRate, max = maxChargeRate);
    Real charge(unit = "kWh", start = startingCharge, max = capacity * 1.01);
    Real chargeRate_toGrid(unit = "kWh");
    Real chargeRate_toLoad(unit = "kWh");
    Boolean present(start = false);
    Boolean not_present(start = true);
  equation
    chargeRate_toGrid = chargeRate;
    chargeRate_toLoad = chargeDissipation * chargeRate;
    der(charge) = chargeRate_toLoad / 60.0;
  end PHEV;

  class HouseData
    import Modelica;
    Real consumption(unit = "kW");
    Real production(unit = "kW");
    Real PHEV_charge(unit = "kWh");
    Real PHEV_chargeRate(unit = "kW");
    Real PHEV_next_hours(unit = "h");
  protected
    /* 1: time, 2: consumption, 3: production, 4: phev initial charge state, 5: phev charge (kWh), 6: phev next hours connected */
    /* the smoothness is Modelica.Blocks.Types.Smoothness.[ConstantSegments|LinearSegments] */
    /* the LUT is not continuous, so ContinuousDerivative is not applicable */
    Modelica.Blocks.Sources.CombiTimeTable LUT(tableOnFile = true, smoothness = Modelica.Blocks.Types.Smoothness.LinearSegments, columns = {2, 3, 4, 5, 6}, tableName = "table", fileName = LUT_path);
    parameter String LUT_path = "LUT_profiles.txt";
  equation
    consumption = LUT.y[1];
    production = LUT.y[2];
    PHEV_charge = LUT.y[3];
    PHEV_chargeRate = LUT.y[4];
    PHEV_next_hours = LUT.y[5];
  end HouseData;
equation
  myCar.present = not HouseSim.PHEV_next_hours == 0;
  myCar.not_present = HouseSim.PHEV_next_hours == 0;
  myCar.chargeRate = HouseSim.PHEV_chargeRate;
  mainBattery.chargeRate = 0;
  when myCar.not_present then
    reinit(myCar.charge, 0);
  end when;
  when myCar.present then
    reinit(myCar.charge, HouseSim.PHEV_charge);
  end when;
  energyConsumption = HouseSim.consumption - HouseSim.production + mainBattery.chargeRate_toGrid + myCar.chargeRate_toGrid;
  annotation(experiment(StartTime = 0, StopTime = 12810, Tolerance = 1e-06, Interval = 1));
end HouseLUT;