model TestServer
  /* The server sends every [comms_time_int] units of the [time] var. */
  parameter Real comms_time_int = 60.0;
  /* The simulation sends data to and asks data from the server every [step_time_int] units of the [time] var. */
  parameter Real step_time_int = 1.0;
  /* The following two parameters set how much time the server waits to get an answer from the controller. [stepLen] sets how many seconds the server waits to check answer availability, [intervalLen] sets how many seconds the server waits in total to get an answer from the controller. */
  parameter Integer stepLen(unit = "s") = 1;
  parameter Integer intervalLen(unit = "s") = 3600;
  Real energyConsumption(unit = "kW");
  TestServer.HouseData HouseSim;
  TestServer.Battery mainBattery(capacity = 4, minChargeRate = -2, maxChargeRate = 2, startingCharge = 2, chargeDissipation = 0.98, dischargeDissipation = 0.82);

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
    input Real o;
    input String n;
    input Real t;
    output Real y;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end getOM;

  function sendOM
    input Real x;
    input String n;
    input Real t;
    output Real y;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end sendOM;

  function startServers
    input Real t;
    input Real sr;
    input Integer ss;
    input Integer st;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end startServers;
initial algorithm
  startServers(time, comms_time_int, stepLen, intervalLen);
  sendOM(pre(energyConsumption), "energy", time);
  sendOM(pre(HouseSim.consumption), "consumption", time);
  sendOM(pre(HouseSim.production), "production", time);
  sendOM(pre(mainBattery.charge), "battery", time);
equation
  when {mod(time, step_time_int) == 0.0} then
    sendOM(pre(energyConsumption), "energy", time);
    sendOM(pre(HouseSim.consumption), "consumption", time);
    sendOM(pre(HouseSim.production), "production", time);
    sendOM(pre(mainBattery.charge), "battery", time);
    mainBattery.chargeRate = getOM(pre(mainBattery.chargeRate), "battery", time);
  end when;
  energyConsumption = HouseSim.consumption - HouseSim.production + mainBattery.chargeRate_toGrid;
  annotation(experiment(StartTime = 0, StopTime = 16800, Tolerance = 1e-06, Interval = 1));
end TestServer;