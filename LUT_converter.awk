BEGIN { 
    start = 0;
    time = 0;
    FS = ",";
    missing_phev = 0;
    missing_phev_last = 0;
}

{
    if (start == 0) {
        start++;
    }
    else {
        gsub(/[-:]/," ",$1);
        gsub(/[-:]/," ",$2);
        d2=mktime($2);
        d1=mktime($1);
        time += (d2-d1)/60;
        missing_phev = (!$5) || (!$6);
        if (missing_phev) {
            $5 = 0;
            $6 = 0;
        }
        if (!missing_phev && missing_phev_last) {
            print time "\t" $3 "\t" $4 "\t" 0 "\t" 0 "\t" 0;
        }
        print time  "\t" $3 "\t" $4 "\t" $5 "\t" $6 "\t" $7;
        missing_phev_last = missing_phev;
    }
}
