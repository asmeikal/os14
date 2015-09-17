model TestServer
  Real energyConsumption(unit = "kW");
  TestServer.HouseData HouseSim annotation(Placement(visible = true, transformation(origin = {-43, 53}, extent = {{-27, -27}, {27, 27}}, rotation = 0)));
  TestServer.Battery mainBattery(capacity = 4, minChargeRate = -2, maxChargeRate = 2, startingCharge = 2, chargeDissipation = 0.98, dischargeDissipation = 0.82) annotation(Placement(visible = true, transformation(origin = {56, 40}, extent = {{-20, -20}, {20, 20}}, rotation = 0)));
  TestServer.Sockets SocketsInterface;

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
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}, preserveAspectRatio = true, initialScale = 0.1, grid = {2, 2}), graphics = {Rectangle(origin = {0, 85}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid, extent = {{-15, -5}, {15, 5}}), Rectangle(origin = {0, 0}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid, extent = {{-50, -90}, {50, 80}})}), Diagram(coordinateSystem(extent = {{-100, -100}, {100, 100}}, preserveAspectRatio = true, initialScale = 0.1, grid = {2, 2})));
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
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}, preserveAspectRatio = true, initialScale = 0.1, grid = {2, 2}), graphics = {Rectangle(origin = {0, 0}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid, extent = {{-90, 40}, {90, -90}}), Polygon(origin = {0, 40}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid, points = {{-90, 0}, {0, 50}, {90, 0}})}));
  end HouseData;

  class Sockets
    Integer control_out(start = 0);
    Integer control_in;
    Real MEAS_energy(unit = "kW");
    Real MEAS_consumption(unit = "kW");
    Real MEAS_production(unit = "kW");
    Real MEAS_battery(unit = "kWh");
    //    Real MEAS_phev(unit = "kWh");
    //    Real MEAS_phev_ready_hours(unit = "h");
    Real CMDS_battery(unit = "kW");
    //    Real CMDS_phev(unit = "kW");

    function getOM
      input Real o;
      input String n;
      input Real t;
      output Real y;
    
      external y = getOM(o, n, t) annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end getOM;

    function getOMcontrol
      input Integer o;
      input Real t;
      output Integer y;
    
      external y = getOMcontrol(o, t) annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end getOMcontrol;

    function sendOM
      input Real x;
      input String n;
      input Real t;
      output Real y;
    
      external y = sendOM(x, n, t) annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end sendOM;

    function sendOMcontrol
      input Integer x;
      input Real t;
      output Integer y;
    
      external y = sendOMcontrol(x, t) annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end sendOMcontrol;

    function startServers
      input Real t;
      output Real y;
    
      external y = startServers(t) annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end startServers;
  initial algorithm
    startServers(time);
    control_out := control_out + 1;
    control_out := Sockets.sendOMcontrol(control_out, time);
    Sockets.sendOM(MEAS_energy, "energy", time);
    Sockets.sendOM(MEAS_consumption, "consumption", time);
    Sockets.sendOM(MEAS_production, "production", time);
    Sockets.sendOM(MEAS_battery, "battery", time);
    control_in := Sockets.getOMcontrol(control_in, time);
    CMDS_battery := Sockets.getOM(CMDS_battery, "battery", time);
  algorithm
    when mod(time, 60.0) == 0.0 then
      control_out := control_out + 1;
      control_out := Sockets.sendOMcontrol(control_out, time);
      Sockets.sendOM(MEAS_energy, "energy", time);
      Sockets.sendOM(MEAS_consumption, "consumption", time);
      Sockets.sendOM(MEAS_production, "production", time);
      Sockets.sendOM(MEAS_battery, "battery", time);
      control_in := Sockets.getOMcontrol(control_in, time);
      CMDS_battery := Sockets.getOM(CMDS_battery, "battery", time);
    end when;
  end Sockets;
equation
  connect(energyConsumption, SocketsInterface.MEAS_energy);
  connect(HouseSim.consumption, SocketsInterface.MEAS_consumption);
  connect(HouseSim.production, SocketsInterface.MEAS_production);
  connect(mainBattery.charge, SocketsInterface.MEAS_battery);
  mainBattery.chargeRate = SocketsInterface.CMDS_battery;
  energyConsumption = HouseSim.consumption - HouseSim.production;
  annotation(experiment(StartTime = 0, StopTime = 128000, Tolerance = 1e-06, Interval = 0.1));
end TestServer;