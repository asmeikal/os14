model TestServerLog
  TestServerLog.HouseData HouseSim;
  Real sp;
  Real sc;

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
    Modelica.Blocks.Sources.CombiTimeTable LUT(tableOnFile = true, smoothness = Modelica.Blocks.Types.Smoothness.ConstantSegments, columns = {2, 3, 4, 5, 6}, tableName = "table", fileName = LUT_path);
    parameter String LUT_path = "LUT_profiles.txt";
  equation
    consumption = LUT.y[1];
    production = LUT.y[2];
    PHEV_charge = LUT.y[3];
    PHEV_chargeRate = LUT.y[4];
    PHEV_next_hours = LUT.y[5];
  end HouseData;

  function myWrite
    input Real x;
    input String n;
    input Real t;
    output Real y;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end myWrite;

  function start
    input Real t;
  
    external "C"  annotation(Library = "libSocketsModelica.a", Include = "#include \"libSocketsModelica.h\"");
  end start;
initial algorithm
  start(time);
algorithm
  sc := TestServerLog.myWrite(HouseSim.consumption, "consumption", time);
  sp := TestServerLog.myWrite(HouseSim.production, "production", time);
  annotation(experiment(StartTime = 0, StopTime = 120, Tolerance = 1e-06, Interval = 0.1));
end TestServerLog;