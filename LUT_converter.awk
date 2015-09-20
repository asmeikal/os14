BEGIN { 
    start = 0;
    time = 0;
    FS = ",";
    missing_phev = 0;
    missing_phev_last = 1;
    phev_charge_last = 0;
    phev_chargeRate_last = 0;
}

{
    if (start == 0) {
        start++;
    }
    else {
        missing_phev = (!$5) || (!$6);

        gsub(/[-:]/," ",$1);
        gsub(/[-:]/," ",$2);
        d2=mktime($2);
        d1=mktime($1);
        if (missing_phev) {
            phev_charge = 0;
            phev_chargeRate = 0;
            phev_hours = 0
        }
        else {
            phev_charge = $5;
            phev_chargeRate = $6;
            phev_hours = $7;
        }
        # discontinuity: car arrives now
        # so at current time we get from 0 to current values
        if (!missing_phev && missing_phev_last) {
            print time "\t" $3 "\t" $4 "\t" 0 "\t" 0 "\t" 0;
        }
        # discontinuity: car leaves now
        # so at current time we get from old values to 0
        # phev_charge should be something else, but I don't have enough info
        if(missing_phev && !missing_phev_last) {
            print time  "\t" $3 "\t" $4 "\t" phev_charge_last "\t" phev_chargeRate_last "\t" 0;
        }
        print time  "\t" $3 "\t" $4 "\t" phev_charge "\t" phev_chargeRate "\t" phev_hours;
        missing_phev_last = missing_phev;
        phev_charge_last = phev_charge;
        phev_chargeRate_last = phev_chargeRate;
        time += (d2-d1)/60;
    }
}
