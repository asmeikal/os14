model TestServer
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

  class HouseData
    import Modelica;
    Real consumption(unit = "kW");
    Real production(unit = "kW");
    Real PHEV_charge(unit = "kWh");
    Real PHEV_chargeRate(unit = "kW");
    Real PHEV_next_hours(unit = "h");
  protected
    /* 1: time, 2: consumption, 3: production, 4: phev initial charge state, 5: phev charge (kWh), 6: phev next hours connected */
    /* an alternative for the smoothness is Modelica.Blocks.Types.Smoothness.ConstantSegments */
    /* the LUT is not continuous, so ContinuousDerivative is not applicable */
    Modelica.Blocks.Sources.CombiTimeTable LUT(tableOnFile = true, smoothness = Modelica.Blocks.Types.Smoothness.LinearSegments, columns = {2, 3, 4, 5, 6}, tableName = "table", fileName = LUT_path);
    parameter String LUT_path = "LUT_profiles.txt";
  equation
    connect(consumption, LUT.y[1]);
    connect(production, LUT.y[2]);
    connect(PHEV_charge, LUT.y[3]);
    connect(PHEV_chargeRate, LUT.y[4]);
    connect(PHEV_next_hours, LUT.y[5]);
  end HouseData;

  function getOM
    input Real t;
    input String n;
    output Real y;
  
    external y = getOM(n) annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end getOM;

  function getOMcontrol
    input Real t;
    output Integer y;
  
    external y = getOMcontrol() annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end getOMcontrol;

  function sendOM
    input Real x;
    input String n;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end sendOM;

  function sendOMcontrol
    input Integer x;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end sendOMcontrol;

  function startServers

    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end startServers;

  HouseData HouseSim;
  Real energyConsumption(unit = "kW");
  Battery mainBattery(capacity = 4, minChargeRate = -2, maxChargeRate = 2, startingCharge = 2, chargeDissipation = 0.98, dischargeDissipation = 0.82);
  Integer control(start = 0);
  Integer receivedControl;
initial algorithm
  startServers();
  control := control + 1;
  sendOMcontrol(control);
  sendOM(mainBattery.charge, "battery");
  sendOM(HouseSim.consumption, "consumption");
  sendOM(HouseSim.production, "production");
  sendOM(energyConsumption, "energy");
  receivedControl := getOMcontrol(time);
  mainBattery.chargeRate := getOM(time, "battery");
algorithm
  when mod(time, 60.0) == 0.0 then
    control := control + 1;
    sendOMcontrol(control);
    sendOM(mainBattery.charge, "battery");
    sendOM(HouseSim.consumption, "consumption");
    sendOM(HouseSim.production, "production");
    sendOM(energyConsumption, "energy");
    receivedControl := getOMcontrol(time);
    mainBattery.chargeRate := getOM(time, "battery");
  end when;
equation
  energyConsumption = HouseSim.consumption - HouseSim.production + mainBattery.chargeRate_toGrid;
end TestServer;