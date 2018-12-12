set argc [llength $argv]
if { $argc != 2 } {
    puts ""
    set errmsg "The script requires two arguments: CL_NAME and TEST_NAME"
    catch {common::send_msg_id "BSG-000" "ERROR" $errmsg}
    return 1
}

set_property verilog_define "CL_NAME=[lindex $argv 0] TEST_NAME=[lindex $argv 1]" [get_filesets sim_1]
update_compile_order -fileset sim_1
launch_simulation -mode behavioral -scripts_only

exit
