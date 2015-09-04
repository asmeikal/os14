class Appliance
  parameter Real maxChargeRate;
  parameter Real minChargeRate;
  Real chargeRate(unit = "kW", min = minChargeRate, max = maxChargeRate);
end Appliance;