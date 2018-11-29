proc setup { name cl_dir} {
    if { $name == "" } {
	puts ""
	set errmsg "The script requires a non-empty name"
	catch {common::send_msg_id "BSG-001" "ERROR" $errmsg}
	return 1
    }

    if { $cl_dir == "" } {
	puts ""
	set errmsg "The script requires a non-empty path for argument cl_dir"
	catch {common::send_msg_id "BSG-002" "ERROR" $errmsg}
	return 1
    }

    if { [file exists $cl_dir] == 0 } {
	puts ""
	set errmsg "The directory $cl_dir does not exist!"
	catch {common::send_msg_id "BSG-003" "ERROR" $errmsg}
	return 1
    }

    create_project $name $name -force
    aws::make_ipi -inline_examples cl_hello_world_ref
    remove_files -fileset sim_1 $cl_dir/$name/$name.srcs/sim_1/imports/tests/test_hello_world.sv
    remove_files {*/hello_world.v}

    add_files -fileset sim_1 $cl_dir/testbenches/rtlsim/test_cl.sv


    delete_bd_objs [get_bd_nets]
    delete_bd_objs [get_bd_intf_nets f1_inst_M_AXI_OCL]
    delete_bd_objs [lrange [get_bd_cells] 1 end]

    puts " *** Working around F1 Build Flow Issues *** "
    exec cat cl.tcl | sed "s/set S_SH/# set S_SH/" \
    	            | sed "s/set f1_inst/# set f1_inst/" \
                    | sed "s/\$f1_inst/\[get_bd_cell f1_inst\]/" \
                    | sed "s/connect_bd_intf_net -intf_net f1_inst_S_SH/\
                           # connect_bd_intf_net -intf_net f1_inst_S_SH/" \
    	            > /tmp/cl.tcl
    source /tmp/cl.tcl -quiet

    create_root_design ""
    save_bd_design

    update_compile_order -fileset sources_1
    update_compile_order -fileset sim_1
    set_property USED_IN_SIMULATION 0 [get_files -regexp -of_objects [get_files -regexp -of_objects [get_filesets sources_1] {.*bd_.*\.bd}] {.*\.bmm}]
    set_property USED_IN_IMPLEMENTATION 0 [get_files -regexp -of_objects [get_files -regexp -of_objects [get_filesets sim_1] {.*bd_.*\.bd}] {.*\.bmm}]
    set_property SCOPED_TO_CELLS {card/fpga/sh/DDR4_3/inst/u_ddr4_mem_intfc/u_ddr_cal_riu/mcs0/inst/microblaze_I} [get_files -regexp -of_objects [get_filesets sim_1] {.*data/mb_bootloop_le\.elf}]

    set_property verilog_define "CL_NAME=cl_top TEST_NAME=test_cl" [get_filesets sim_1]
}

set argc [llength $argv]
if { $argc != 2 } {
    puts ""
    set errmsg "The script requires a target project name and path to\
the directory of this project"
    catch {common::send_msg_id "BSG-000" "ERROR" $errmsg}
    return 1
}

setup [lindex $argv 0] [lindex $argv 1]

exit
