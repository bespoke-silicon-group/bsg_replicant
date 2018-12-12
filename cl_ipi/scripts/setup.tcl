proc setup { target cl_dir new_ip_paths rtlsim_file cosim_file rtlsim_module} {
    if { $target == "" } {
	puts ""
	set errmsg "The script requires a non-empty target"
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

    create_project $target $target -force
    aws::make_ipi -inline_examples cl_hello_world_ref
    remove_files -fileset sim_1 $cl_dir/$target/$target.srcs/sim_1/imports/tests/test_hello_world.sv
    remove_files {*/hello_world.v}

    set cur_ip_paths [get_property ip_repo_paths [current_project] ]

    set_property ip_repo_paths [lappend new_ip_paths $cur_ip_paths] [current_project]

    update_ip_catalog

    add_files -fileset sim_1 $rtlsim_file
    add_files -fileset sim_1 $cosim_file

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

    set_property verilog_define "CL_NAME=cl_top TEST_NAME=$rtlsim_module" [get_filesets sim_1]
}

set argc [llength $argv]
if { $argc != 6 } {
    puts ""
    set errmsg "The script requires a target name, project root path, ip path, rtlsim path, cosim path, and rtlsim top module name"
    catch {common::send_msg_id "BSG-000" "ERROR" $errmsg}
    return 1
}

setup [lindex $argv 0] [lindex $argv 1] [lindex $argv 2] [lindex $argv 3] [lindex $argv 4] [lindex $argv 5]

exit
