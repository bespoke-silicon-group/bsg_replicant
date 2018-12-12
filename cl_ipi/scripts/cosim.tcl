set argc [llength $argv]
if { $argc != 1 } {
    puts ""
    set errmsg "The script requires 1 argument: Testbench Module Name (TEST_NAME)"
    catch {common::send_msg_id "BSG-000" "ERROR" $errmsg}
    return 1
}

set_property verilog_define "CL_NAME=cl_top TEST_NAME=[lindex $argv 0]" [get_filesets sim_1]
update_compile_order -fileset sim_1
launch_simulation -mode behavioral -scripts_only

exit
