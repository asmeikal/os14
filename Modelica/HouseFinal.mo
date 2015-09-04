model House
  parameter Real enreqUpperLimit = 24;
  parameter Real enreqLowerLimit = -6;
  HouseData HouseSim;
  Battery mainBattery(capacity = 4, charge = 2, maxChargeRate = 2, minChargeRate = 2, chargeDissipation = 0.98, dischargeDissipation = 0.82);
  Battery PHEV(capacity = 16, maxChargeRate = 13, minChargeRate = 0, chargeDissipation = 0.876, dischargeDissipation = 0);
  Real energyRequest(min = enreqLowerLimit, max = enreqUpperLimit, unit = "kW");
protected
  Boolean onTheHour(start = false);
  Integer control(start = 0);
  Integer controlReceived(start = 0);
initial algorithm
  startServer();
  PHEV.charge := HouseSim.PHEV_initial_charge_state;
algorithm
  onTheHour := mod(time, 60.0) == 0.0;
  when HouseSim.PHEV_arrived then
    PHEV.charge := HouseSim.PHEV_initial_charge_state;
  end when;
  when onTheHour then
    control := control + 1;
    controlReceived := getOM("control");
    assert(controlReceived == control, "Wrong control received.");
    mainBattery.chargeRate := getOM("battery_charge_rate");
    PHEV.chargeRate := getOM("battery_charge_rate");
    sendOM(control, "control");
    sendOM(HouseSim.consumption, "consumption");
    sendOM(HouseSim.production, "production");
    sendOM(HouseSim.PHEV_initial_charge_state, "PHEV_initial_charge_state");
    sendOM(HouseSim.PHEV_charge_rate, "PHEV_charge_rate");
    sendOM(HouseSim.PHEV_next_hours_connected, "PHEV_next_hours_connected");
  end when;
equation
  energyRequest = HouseSim.consumption - HouseSim.production + PHEV.chargeRate_onGrid + mainBattery.chargeRate_onGrid;
end House;