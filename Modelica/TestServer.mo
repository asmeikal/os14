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
    Real charge(unit = "kWh", min = capacity * 0.19, max = capacity * 1.01, start = startingCharge);
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
    //    Integer control_out(start = 0);
    //    Integer control_in;
    Real MEAS_energy(unit = "kW");
    Real MEAS_consumption(unit = "kW");
    Real MEAS_production(unit = "kW");
    Real MEAS_battery(unit = "kWh");
    //    Real MEAS_phev(unit = "kWh");
    //    Real MEAS_phev_ready_hours(unit = "h");
    Real CMDS_battery(unit = "kW");
    //    Real CMDS_phev(unit = "kW");

    function getOM_NB
      input Real o;
      input String n;
      input Real t;
      output Real y;
    
      external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end getOM_NB;

    function getOMcontrol_NB
      input Integer o;
      input Real t;
      output Integer y;
    
      external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end getOMcontrol_NB;

    function sendOM
      input Real x;
      input String n;
      input Real t;
      output Real y;
    
      external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end sendOM;

    function sendOMcontrol
      input Integer x;
      input Real t;
      output Integer y;
    
      external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end sendOMcontrol;

    function startServers
      input Real t;
      output Real y;
    
      external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
    end startServers;

    function allWork
      input Real cb;
      input Real me;
      input Real mc;
      input Real mp;
      input Real mb;
      input Real t;
      output Real y;
    algorithm
      y := getOM_NB(cb, "battery", t);
      sendOM(me, "energy", t);
      sendOM(mc, "consumption", t);
      sendOM(mp, "production", t);
      sendOM(mb, "battery", t);
    end allWork;
  algorithm
    startServers(time);
    CMDS_battery := allWork(CMDS_battery, MEAS_energy, MEAS_consumption, MEAS_production, MEAS_battery, time);
  end Sockets;
equation
  SocketsInterface.MEAS_energy = energyConsumption;
  SocketsInterface.MEAS_consumption = HouseSim.consumption;
  SocketsInterface.MEAS_production = HouseSim.production;
  SocketsInterface.MEAS_battery = mainBattery.charge;
  mainBattery.chargeRate = SocketsInterface.CMDS_battery;
  energyConsumption = HouseSim.consumption - HouseSim.production + mainBattery.chargeRate_toGrid;
  annotation(experiment(StartTime = 0, StopTime = 48000, Tolerance = 1e-06, Interval = 0.1));
end TestServer;