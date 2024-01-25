#include <bsg_manycore.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_config_pod.h>

#include <bsg_manycore_regression.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>

static int read_program_file(const char *file_name, unsigned char **file_data, size_t *file_size)
{
        struct stat st;
        FILE *f;
        int r;
        unsigned char *data;

        if ((r = stat(file_name, &st)) != 0) {
                bsg_pr_err("could not stat '%s': %m\n", file_name);
                return HB_MC_FAIL;
        }

        if (!(f = fopen(file_name, "rb"))) {
                bsg_pr_err("failed to open '%s': %m\n", file_name);
                return HB_MC_FAIL;
        }

        if (!(data = (unsigned char *) malloc(st.st_size))) {
                bsg_pr_err("failed to read '%s': %m\n", file_name);
                fclose(f);
                return HB_MC_FAIL;
        }

        if ((r = fread(data, st.st_size, 1, f)) != 1) {
                bsg_pr_err("failed to read '%s': %m\n", file_name);
                fclose(f);
                free(data);
                return HB_MC_FAIL;
        }

        fclose(f);
        *file_data = data;
        *file_size = st.st_size;
        return HB_MC_SUCCESS;
}



// Main;
int test_loader(int argc, char **argv) {

  int err, r = HB_MC_FAIL;
  hb_mc_dimension_t tg;
  char *bin_path, *test_name;
  struct arguments_spmd args = {NULL, NULL, 0, 0};

  argp_parse (&argp_spmd, argc, argv, 0, 0, &args);
  bin_path = args.path;
  test_name = args.name;
  tg.x = args.tg_x;
  tg.y = args.tg_y;

  printf("Reading from file: %s\n", bin_path);
  printf("Pod group dim: %d %d\n", POD_GROUP_X, POD_GROUP_Y);
  printf("Tile group dim: %d %d\n", tg.x, tg.y);

  // read program;
  unsigned char *program_data;
  size_t program_size;
  read_program_file(bin_path, &program_data, &program_size);


  // init manycore;
  hb_mc_manycore_t manycore = {0}, *mc = &manycore;
  hb_mc_manycore_init(&manycore, test_name, 0);
  const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
  hb_mc_coordinate_t pod;  

  // Initialize Pods;
  hb_mc_coordinate_t origin, target;  
  hb_mc_config_foreach_pod(pod,cfg) {
    // Skip pods;
    if ((pod.x >= POD_GROUP_X) || (pod.y >= POD_GROUP_Y)) {
      //printf("Skipping pod (%d, %d)\n", pod.x, pod.y);
      continue;
    }

    // enable dram? 
    int enable_dram = 0;

    // initialize tiles;
    printf("Loading to pod (%d %d)\n", pod.x, pod.y);
    origin = hb_mc_config_pod_vcore_origin(cfg, pod);
    target = origin;

    foreach_coordinate(target, origin, tg) {
      // freeze tile;
      hb_mc_tile_freeze(mc, &target);
      // load program;
      hb_mc_loader_load(program_data, program_size, mc, &default_map, &target, 1);
      // set tg origin;
      hb_mc_tile_set_origin(mc, &target, &origin);
      hb_mc_tile_set_origin_registers(mc, &target, &origin);
    }
  }

  // Unfreeze tiles;
  hb_mc_config_foreach_pod(pod,cfg) {
    if ((pod.x >= POD_GROUP_X) || (pod.y >= POD_GROUP_Y)) {
      continue;
    }
    printf("Unfreezing pod (%d %d)\n", pod.x, pod.y);
    origin = hb_mc_config_pod_vcore_origin(cfg, pod);
    target = origin;
    foreach_coordinate(target, origin, tg) {
      hb_mc_tile_unfreeze(mc, &target);
    }
  }


  // Wait for Finish;
  printf("Waiting for pods to finish...\n");
  int done = 0;
  int fail = 0;
  while (done < tg.x * tg.y *POD_GROUP_X * POD_GROUP_Y) {
    // dequeue packet;
    hb_mc_packet_t pkt;
    hb_mc_manycore_packet_rx(mc, &pkt, HB_MC_FIFO_RX_REQ, -1);
   
    // check packet;
    hb_mc_coordinate_t src;
    src.x = hb_mc_request_packet_get_x_src(&pkt.request); 
    src.y = hb_mc_request_packet_get_y_src(&pkt.request); 
  
    switch (hb_mc_request_packet_get_epa(&pkt.request)) {
      // FINISH;
      case 0xEAD0:
        printf("Received FINISH packet from (%d %d)\n", src.x, src.y);
        done++;
        break;
      // FAIL;
      case 0xEAD8:
        printf("Received FAIL packet from (%d %d)\n", src.x, src.y);
        done++;
        fail++;
        break;
      default: break;
    }
  }
 

  // End; 
  hb_mc_manycore_exit(mc);
  if (fail > 0) {
    return HB_MC_FAIL;
  } else {
    return HB_MC_SUCCESS;
  }

}


declare_program_main("SPMD Loader", test_loader);
