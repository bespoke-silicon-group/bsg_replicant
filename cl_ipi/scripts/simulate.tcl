proc simulate { njobs } {
    if { $njobs == "" } {
	puts ""
	set errmsg "The script requires a non-empty number of jobs"
	catch {common::send_msg_id "BSG-001" "ERROR" $errmsg}
	return 1
    }

    launch_simulation -mode behavioral
}

set argc [llength $argv]
if { $argc != 1 } {
    puts ""
    set errmsg "The script requires a value for number of jobs (njobs)"
    catch {common::send_msg_id "BSG-000" "ERROR" $errmsg}
    return 1
}

simulate [lindex $argv 0] 

exit
