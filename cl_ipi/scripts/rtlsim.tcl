proc rtlsim {test_name} {

    set_property verilog_define "CL_NAME=cl_top TEST_NAME=$test_name" [get_filesets sim_1]
    launch_simulation -mode behavioral
}

set argc [llength $argv]
if { $argc != 1 } {
    puts ""
    set errmsg "The script requires one argument: Testbench Module Name (TEST_NAME)"
    catch {common::send_msg_id "BSG-000" "ERROR" $errmsg}
    return 1
}

rtlsim [lindex $argv 0]

exit
