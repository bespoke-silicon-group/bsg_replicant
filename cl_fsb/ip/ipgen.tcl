if { [info exists ::env(HDK_SHELL_DIR)] } {
        set HDK_SHELL_DIR $::env(HDK_SHELL_DIR)
        puts "Using Shell directory $HDK_SHELL_DIR";
} else {
        puts "Error: HDK_SHELL_DIR environment variable not defined ! ";
        puts "Run the hdk_setup.sh script from the root directory of aws-fpga";
        exit 2
}

#Set the Device Type
source $HDK_SHELL_DIR/build/scripts/device_type.tcl

set PART [DEVICE_TYPE]
set IP_PATH ./

set argc [llength $argv]
if { $argc != 1 } {
    puts ""
    set errmsg "The script requires a target IP directory name"
    catch {common::send_msg_id "BSG-000" "ERROR" $errmsg}
    return 1
}

set IP_TARGET [lindex $argv 0]
set IP_FILE $IP_PATH/$IP_TARGET/$IP_TARGET.xci

if { [file exists $IP_FILE] == 0 } {
    puts ""
    set errmsg "Could not find file: $IP_TARGET."
    catch {common::send_msg_id "BSG-001" "ERROR" $errmsg}
    return 1
}

set_part $PART
add_file $IP_FILE -copy_to . -force
generate_target simulation [get_ips $IP_TARGET]
generate_target synthesis [get_ips $IP_TARGET]
generate_target instantiation_template [get_ips $IP_TARGET]
