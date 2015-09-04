class HouseData
  Real consumption(unit = "kW");
  Real production(unit = "kW");
  Real PHEV_initial_charge_state(unit = "kWh");
  Real PHEV_charge_rate(unit = "kW");
  Real PHEV_next_hours_connected(unit = "h");
  Boolean PHEV_present;
  Boolean PHEV_arrived;
protected
  /* 1: time, 2: consumption, 3: production, 4: phev initial charge state, 5: phev charge (kWh), 6: phev next hours connected */
  /* an alternative for the smoothness is Modelica.Blocks.Types.Smoothness.ConstantSegments */
  /* the LUT is not continuous, so ContinuousDerivative is not applicable */
  Modelica.Blocks.Sources.CombiTimeTable LUT(tableOnFile = true, smoothness = Modelica.Blocks.Types.Smoothness.LinearSegments, columns = {2, 3, 4, 5, 6}, tableName = "table", fileName = LUT_path);
  parameter String LUT_path = "../LUT_profiles.txt";
  Boolean PHEV_present_last(start = false);
algorithm
  PHEV_present := not LUT.y[3] == 0;
  PHEV_arrived := PHEV_present and not PHEV_present_last;
  PHEV_present_last := PHEV_present;
equation
  connect(consumption, LUT.y[1]);
  connect(production, LUT.y[2]);
  connect(PHEV_initial_charge_state, LUT.y[3]);
  connect(PHEV_charge_rate, LUT.y[4]);
  connect(PHEV_next_hours_connected, LUT.y[5]);
end HouseData;