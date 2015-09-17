model HouseLUT
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
  annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}, preserveAspectRatio = true, initialScale = 0.1, grid = {2, 2}), graphics = {Rectangle(origin = {0, 0}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid, extent = {{-90, 40}, {90, -90}}), Polygon(origin = {0, 40}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid, points = {{-90, 0}, {0, 50}, {90, 0}})}), experiment(StartTime = 0, StopTime = 128000, Tolerance = 1e-6, Interval = 0.1));
end HouseLUT;