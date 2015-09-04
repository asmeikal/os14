class Appliance
  parameter Real maxChargeRate;
  parameter Real minChargeRate;
  Real chargeRate(unit = "kWh", min = minChargeRate, max = maxChargeRate);
end Appliance;