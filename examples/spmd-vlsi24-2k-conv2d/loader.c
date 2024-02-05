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
#include <time.h>

#define abs(x) ((x) < 0 ? -1 * (x) : (x))
float expected[196] = {1.4707666635513306, 0.8525369763374329, 1.084539532661438, 2.1422932147979736, 2.6825172901153564, 3.3721437454223633, 2.9203906059265137, 2.3674933910369873, 2.504912853240967, 1.408334493637085, 1.1817741394042969, 0.7141904830932617, 1.4250218868255615, 2.65584135055542, 2.02791428565979, 2.0679965019226074, 2.5623819828033447, 3.2818081378936768, 3.176321506500244, 2.7241737842559814, 2.6618270874023438, 1.636821985244751, 1.9077188968658447, 1.2800145149230957, 1.1178990602493286, 1.7616547346115112, 2.3815526962280273, 2.4628968238830566, 2.3618571758270264, 2.370368003845215, 2.4687514305114746, 3.008419990539551, 3.3217217922210693, 2.221870183944702, 2.1705198287963867, 2.1776797771453857, 2.5522079467773438, 2.8423984050750732, 2.6667566299438477, 2.204028844833374, 1.706357479095459, 2.1314239501953125, 2.65444278717041, 3.024667978286743, 2.8422632217407227, 2.513538122177124, 2.1708409786224365, 1.9707183837890625, 1.9725068807601929, 2.3389039039611816, 2.3452064990997314, 2.538832187652588, 2.9644112586975098, 2.8472952842712402, 2.3751895427703857, 1.2437812089920044, 2.5369033813476562, 1.5873754024505615, 2.2065672874450684, 2.8888189792633057, 2.934333562850952, 1.685279369354248, 1.6459546089172363, 1.738447904586792, 2.142197370529175, 2.26134991645813, 2.3091375827789307, 2.2009921073913574, 1.5549159049987793, 1.9423918724060059, 2.8202240467071533, 2.461594820022583, 1.9560256004333496, 2.1866931915283203, 2.113173007965088, 2.1965255737304688, 1.9826130867004395, 2.4599382877349854, 1.7090624570846558, 2.0237362384796143, 2.0497989654541016, 2.0965609550476074, 1.3875682353973389, 1.4997541904449463, 1.9654804468154907, 1.3668835163116455, 1.815205454826355, 2.368101119995117, 2.4463634490966797, 2.4276797771453857, 2.4563276767730713, 1.9227651357650757, 2.0246729850769043, 2.281961679458618, 2.2670540809631348, 2.8230247497558594, 2.969109058380127, 2.5417768955230713, 2.1390302181243896, 2.494769811630249, 1.7426526546478271, 1.6018683910369873, 1.652336597442627, 1.3636984825134277, 1.6398649215698242, 2.128415584564209, 2.623840093612671, 2.5120701789855957, 2.73907208442688, 1.8623698949813843, 2.3337466716766357, 1.6815637350082397, 1.5940423011779785, 1.4560271501541138, 1.317518949508667, 1.2503975629806519, 1.6158665418624878, 1.9264216423034668, 1.6496964693069458, 2.089055299758911, 2.417607307434082, 2.3456871509552, 3.249114513397217, 2.714674711227417, 2.682305335998535, 2.2874515056610107, 1.7981895208358765, 1.9439541101455688, 1.9349148273468018, 1.546670913696289, 1.8433669805526733, 2.223740577697754, 2.116027355194092, 2.587374448776245, 3.033743381500244, 2.199018716812134, 1.8949567079544067, 2.309756278991699, 1.885750651359558, 3.0864436626434326, 1.8223317861557007, 1.0520597696304321, 1.219002604484558, 1.6959879398345947, 2.1812307834625244, 2.1550722122192383, 2.3165431022644043, 2.3513545989990234, 2.3567240238189697, 2.1084489822387695, 1.0571081638336182, 2.444512128829956, 2.525263547897339, 2.7931623458862305, 2.0446949005126953, 2.117036819458008, 1.571473479270935, 2.1818535327911377, 1.9576886892318726, 2.4474759101867676, 2.160034656524658, 2.244729518890381, 1.859350323677063, 1.7121360301971436, 1.6203736066818237, 2.1656675338745117, 2.957859754562378, 3.520803689956665, 1.8512685298919678, 1.6255028247833252, 1.4815362691879272, 1.6742725372314453, 2.081570863723755, 2.284229278564453, 2.355191469192505, 2.7388598918914795, 3.227872610092163, 2.0006439685821533, 1.9768356084823608, 2.1515674591064453, 2.4046547412872314, 1.9978536367416382, 2.3366847038269043, 2.2138476371765137, 2.018960475921631, 2.0394082069396973, 2.135880708694458, 2.2378997802734375, 2.2407047748565674, 1.9576395750045776, 2.2527122497558594, 2.0668962001800537, 2.2400078773498535, 2.376486301422119, 2.4288387298583984, 1.8917549848556519, };

uint64_t get_us() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * (uint64_t)1000000) + (time.tv_usec);
}

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

  bsg_pr_info("Reading from file: %s\n", bin_path);
  bsg_pr_info("Pod group dim: %d %d\n", POD_GROUP_X, POD_GROUP_Y);
  bsg_pr_info("Tile group dim: %d %d\n", tg.x, tg.y);

  // read program;
  unsigned char *program_data;
  size_t program_size;
  read_program_file(bin_path, &program_data, &program_size);


  // init manycore;
  hb_mc_manycore_t manycore = {0}, *mc = &manycore;
  hb_mc_manycore_init(&manycore, test_name, 0, POD_GROUP_X, POD_GROUP_Y);
  const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
  hb_mc_coordinate_t pod;

  // Initialize Pods;
  hb_mc_coordinate_t origin, target;
  hb_mc_config_foreach_pod(pod,cfg) {
    // Skip pods;
    if ((pod.x >= POD_GROUP_X) || (pod.y >= POD_GROUP_Y)) {
      bsg_pr_info("Skipping pod (%d, %d)\n", pod.x, pod.y);
      continue;
    }

    //if ((pod.x != 0) || (pod.y != 2)) {
    //  bsg_pr_info("Skipping pod (%d, %d)\n", pod.x, pod.y);
    //  continue;
    //}

    // enable dram?
    //int enable_dram = (pod.y < 2);
    int enable_dram = 0;

    // initialize tiles;
    bsg_pr_info("Loading to pod (%d %d)\n", pod.x, pod.y);
    origin = hb_mc_config_pod_vcore_origin(cfg, pod);
    target = origin;

    foreach_coordinate(target, origin, tg) {
      // freeze tile;
      hb_mc_tile_freeze(mc, &target);
      // load program;
      hb_mc_loader_load(program_data, program_size, mc, &default_map, &target, 1, enable_dram);
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
    //if ((pod.x != 0) || (pod.y != 2)) {
    //  bsg_pr_info("Skipping pod (%d, %d)\n", pod.x, pod.y);
    //  continue;
    //}
    bsg_pr_info("Unfreezing pod (%d %d)\n", pod.x, pod.y);
    origin = hb_mc_config_pod_vcore_origin(cfg, pod);
    target = origin;
    foreach_coordinate(target, origin, tg) {
      hb_mc_tile_unfreeze(mc, &target);
    }
  }

  uint32_t output_addrs[2048] = {0};

  // Wait for Finish;
  bsg_pr_info("Waiting for pods to finish...\n");
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

    uint32_t real_x = src.x - 16;
    uint32_t real_y = src.y - 8 - (src.y/16)*8;
    uint32_t index = real_y * 64 + real_x;

    uint32_t data;
    data = hb_mc_request_packet_get_data(&pkt.request);

    //hb_mc_npa_t npa ;
    //uint32_t value;

    switch (hb_mc_request_packet_get_epa(&pkt.request)) {
      // FINISH;
      case 0xEAD0:
        bsg_pr_info("Received FINISH packet from (%d %d %d)\n", src.x, src.y, index);
        done++;
        break;
      // FAIL;
      case 0xEAD8:
        bsg_pr_info("Received FAIL packet from (%d %d)\n", src.x, src.y);
        done++;
        fail++;
        break;

      // TIME;
      case 0xEAD4:
        bsg_pr_info("TIME packet from (%d %d %lu)\n", src.x, src.y, get_us());
        break;

      case 0xEADC:
        output_addrs[index] = data;
        //bsg_pr_info("ADDR packet from (%d %d %u %X)\n", src.x, src.y, data, value);
        break;

      default: break;
    }
  }

  // run the check
  uint32_t errors = 0;

  hb_mc_config_foreach_pod(pod,cfg) {
    bsg_pr_info("Checking pod (%d %d)\n", pod.x, pod.y);
    origin = hb_mc_config_pod_vcore_origin(cfg, pod);
    target = origin;
    foreach_coordinate(target, origin, tg) {
        uint32_t real_x = target.x - 16;
        uint32_t real_y = target.y - 8 - (target.y/16)*8;
        uint32_t index = real_y * 64 + real_x;
        uint32_t addr = output_addrs[index];

        bsg_pr_info("Checking tile (%d %d %u)\n", target.x, target.y);

        hb_mc_npa_t npa;
        uint32_t value;

        for (int i = 0; i < 196; i++) {
            npa = hb_mc_npa(target, addr + (i*4));
            hb_mc_manycore_read32(mc, &npa, &value);
            float* f_res = (float*)(&value);
            if (abs((*f_res) - expected[i]) > 0.0001) {
                errors ++;
            }
        }
    }
  }

  // End;
  hb_mc_manycore_exit(mc);

  if (fail > 0 || errors > 0) {
    return HB_MC_FAIL;
  } else {
    return HB_MC_SUCCESS;
  }

}


declare_program_main("SPMD Loader", test_loader);
