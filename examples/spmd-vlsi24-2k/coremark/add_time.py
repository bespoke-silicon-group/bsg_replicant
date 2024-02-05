confirm_csv = "confirm/confirm.csv"
confirm_fid = open(confirm_csv, "r")

for i,line in enumerate(confirm_fid):
    if i == 0:
        print(line.strip() + ",start_us,end_us,diff_us")
    else:
        lsplit = line.strip().split(",")
        v_mv = lsplit[0]
        osc = lsplit[1]
        first_time, last_time = None, None
        with open(f"confirm/confirm_v{v_mv}_osc{osc}.log", "r") as log:
            for log_line in log:
                if log_line.find("TIME") != -1:
                    if first_time is None:
                        first_time = log_line.strip()
                    last_time = log_line.strip()
        start_us = int(first_time.split()[-1][:-1])
        end_us   = int(last_time.split()[-1][:-1])
        print(line.strip() + f",{start_us},{end_us},{end_us-start_us}")



confirm_fid.close()
