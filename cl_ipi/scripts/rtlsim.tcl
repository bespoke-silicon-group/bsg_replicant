proc rtlsim {cl_name test_name} {

    set_property verilog_define "CL_NAME=$cl_name TEST_NAME=$test_name" [get_filesets sim_1]
    launch_simulation -mode behavioral
}

set argc [llength $argv]
if { $argc != 2 } {
    puts ""
    set errmsg "The script requires two arguments: CL_NAME and TEST_NAME"
    catch {common::send_msg_id "BSG-000" "ERROR" $errmsg}
    return 1
}

rtlsim [lindex $argv 0] [lindex $argv 1]

exit
