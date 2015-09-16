model TestServerSimple
  Real energyConsumption(unit = "kW");
  TestServerSimple.HouseData HouseSim;
  TestServerSimple.Battery mainBattery(capacity = 4, minChargeRate = -2, maxChargeRate = 2, startingCharge = 2, chargeDissipation = 0.98, dischargeDissipation = 0.82);
  Real se;
  Real sp;
  Real sc;
  Real sb;

  class Battery
    parameter Real maxChargeRate(unit = "kW");
    parameter Real minChargeRate(unit = "kW");
    parameter Real startingCharge(unit = "kWh");
    parameter Real chargeDissipation;
    parameter Real dischargeDissipation;
    parameter Real capacity(unit = "kWh");
    Real chargeRate(unit = "kW", min = minChargeRate, max = maxChargeRate, start = 0);
    Real charge(unit = "kWh", min = capacity * 0.19, max = capacity * 1.01, start = startingCharge);
    Real chargeRate_toGrid(unit = "kWh");
    Real chargeRate_toLoad(unit = "kWh");
  equation
    chargeRate_toGrid = if chargeRate >= 0 then chargeRate else chargeRate * dischargeDissipation;
    chargeRate_toLoad = if chargeRate >= 0 then chargeDissipation * chargeRate else chargeRate;
    der(charge) = chargeRate_toLoad / 60;
    //    der(charge) = chargeRate / 60;
    //    der(charge) = 0;
  end Battery;

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

  function getOM_NB
    input Real o;
    input String n;
    input Real t;
    output Real y;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end getOM_NB;

  function sendOM
    input Real x;
    input String n;
    input Real t;
    output Real y;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end sendOM;

  function startServers
    input Real t;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end startServers;
initial algorithm
  TestServerSimple.startServers(time);
algorithm
  mainBattery.chargeRate := TestServerSimple.getOM_NB(mainBattery.chargeRate, "battery", time);
  energyConsumption := HouseSim.consumption - HouseSim.production + mainBattery.chargeRate;
  se := TestServerSimple.sendOM(energyConsumption, "energy", time);
  sc := TestServerSimple.sendOM(HouseSim.consumption, "consumption", time);
  sp := TestServerSimple.sendOM(HouseSim.production, "production", time);
  sb := TestServerSimple.sendOM(mainBattery.charge, "battery", time);
  annotation(experiment(StartTime = 0, StopTime = 120, Tolerance = 1e-06, Interval = 0.1));
end TestServerSimple;