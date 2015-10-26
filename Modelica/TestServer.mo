model TestServer
  Real energyConsumption(unit = "kW");
  TestServer.HouseData HouseSim;
  TestServer.Battery mainBattery(capacity = 4, minChargeRate = -2, maxChargeRate = 2, startingCharge = 2, chargeDissipation = 0.98, dischargeDissipation = 0.82);
  TestServer.PHEV myCar(capacity = 16, maxChargeRate = 13, chargeDissipation = 0.876, dischargeDissipation = 0.0, startingCharge = 0);

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
    der(charge) = chargeRate_toLoad / 60.0;
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
    Boolean not_present(start = true);
    Boolean present(start = false);
  equation
    chargeRate_toGrid = chargeRate;
    chargeRate_toLoad = chargeDissipation * chargeRate;
    der(charge) = if present then chargeRate_toLoad / 60.0 else 0;
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

  function getOM
    input Real old_value;
    input String name;
    input Real time;
    input Integer control;
    output Real result;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end getOM;

  function sendOM
    input Real value;
    input String name;
    input Real time;
    input Integer control;
    output Real result;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end sendOM;

  function startServers
    input Real time;
    input Integer sec_per_step;
    input Integer sec_per_time_int;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end startServers;
protected
  /* The server sends every [comms_time_int] units of the [time] var. */
  parameter Real comms_time_int = 60.0;
  /* The simulation sends data to and asks data from the server every [queries_per_int] units of the [time] var. */
  parameter Integer queries_per_int = 60;
  parameter Integer speed = 1;
  parameter Real step_time = comms_time_int / queries_per_int;
  Integer control(start = 0);
initial algorithm
  control := control + 1;
  startServers(time, queries_per_int, speed);
  sendOM(pre(energyConsumption), "energy", time, control);
  sendOM(pre(HouseSim.consumption), "consumption", time, control);
  sendOM(pre(HouseSim.production), "production", time, control);
  sendOM(pre(mainBattery.charge), "battery", time, control);
  sendOM(pre(myCar.charge), "phev", time, control);
  sendOM(pre(HouseSim.PHEV_next_hours), "phev_ready_hours", time, control);
algorithm
  myCar.not_present := HouseSim.PHEV_next_hours == 0.0;
  myCar.present := not HouseSim.PHEV_next_hours == 0.0;
  when {mod(time, comms_time_int) == 0.0} then
    control := control + 1;
    sendOM(pre(energyConsumption), "energy", time, control);
    sendOM(pre(HouseSim.consumption), "consumption", time, control);
    sendOM(pre(HouseSim.production), "production", time, control);
    sendOM(pre(mainBattery.charge), "battery", time, control);
    sendOM(pre(HouseSim.PHEV_next_hours), "phev_ready_hours", time, control);
    sendOM(if myCar.present then pre(myCar.charge) else -1, "phev", time, control);
  end when;
  when {mod(time, step_time) == 0.0} then
    myCar.chargeRate := getOM(pre(myCar.chargeRate), "phev", time, control);
    mainBattery.chargeRate := getOM(pre(mainBattery.chargeRate), "battery", time, control);
  end when;
equation
  when myCar.not_present then
    reinit(myCar.charge, 0);
    //reinit(myCar.chargeRate, 0);
  end when;
  when myCar.present then
    reinit(myCar.charge, HouseSim.PHEV_charge);
  end when;
  energyConsumption = HouseSim.consumption - HouseSim.production + mainBattery.chargeRate_toGrid + myCar.chargeRate_toGrid;
  annotation(experiment(StartTime = 0, StopTime = 128100, Tolerance = 1e-06, Interval = 1));
end TestServer;