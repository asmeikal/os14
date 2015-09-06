model House
  parameter Real enreqUpperLimit(unit = "kW") = 24;
  parameter Real enreqLowerLimit(unit = "kW") = -6;
  HouseData HouseSim;
  Battery mainBattery(capacity = 4, chargeRate = 1, maxChargeRate = 2, minChargeRate = 2);
  Real energyRequest(min = enreqLowerLimit, max = enreqUpperLimit, unit = "kW");
algorithm
  energyRequest := (HouseSim.consumption - HouseSim.production + mainBattery.chargeRate) / 60;
  mainBattery.chargeRate := sin(time);
end House;