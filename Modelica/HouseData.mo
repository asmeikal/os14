class HouseData
  import Modelica.Blocks.Sources.CombiTimeTable;
  Real consumption(unit = "kW");
  Real production(unit = "kW");
  Real PHEV_charge(unit = "kWh");
  Real PHEV_chargeRate(unit = "kW");
  Real PHEV_next_hours(unit = "h");
  Boolean PHEV_arrived;
  Boolean PHEV_present;
protected
  /* 1: time, 2: consumption, 3: production, 4: phev initial charge state, 5: phev charge (kWh), 6: phev next hours connected */
  /* an alternative for the smoothness is Modelica.Blocks.Types.Smoothness.ConstantSegments */
  /* the LUT is not continuous, so ContinuousDerivative is not applicable */
  CombiTimeTable LUT(tableOnFile = true, smoothness = Modelica.Blocks.Types.Smoothness.LinearSegments, columns = {2, 3, 4, 5, 6}, tableName = "table", fileName = LUT_path);
  parameter String LUT_path = "/home/michele/Developer/Modelica/Progetto/LUT_profiles.txt";
  Boolean PHEV_present_last(start = false);
algorithm
  PHEV_present := not LUT.y[3] == 0;
  PHEV_arrived := PHEV_present and not PHEV_present_last;
  PHEV_present_last := PHEV_present;
equation
  connect(consumption, LUT.y[1]);
  connect(production, LUT.y[2]);
  connect(PHEV_charge, LUT.y[3]);
  connect(PHEV_chargeRate, LUT.y[4]);
  connect(PHEV_next_hours, LUT.y[5]);
end HouseData;