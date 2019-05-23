#ifndef BSG_MANYCORE_NPA_H
#define BSG_MANYCORE_NPA_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_epa.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A Network Physical Address.
 * This type uniquely identifies a physical memory address within the entire manycore fabric.
 * Never access the fields of this struct directly. Instead use the accessor functions.
 * The implementation of this type may change -- that's why you should use the accessors.
 */
typedef struct {
    hb_mc_idx_t x;
    hb_mc_idx_t y;
    hb_mc_epa_t epa;
} hb_mc_npa_t;

/**
 * Get the X coordinate from #npa.
 * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
 * @return the X coordinate of #npa.
 */
static inline hb_mc_idx_t hb_mc_npa_get_x(const hb_mc_npa_t *npa)
{
    return npa->x;
}


/**
 * Get the Y coordinate from #npa.
 * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
 * @return the Y coordinate of #npa.
 */
static inline hb_mc_idx_t hb_mc_npa_get_y(const hb_mc_npa_t *npa)
{
    return npa->y;
}

/**
 * Get the Endpoint Physical Address from #npa.
 * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
 * @return EPA of the NPA.
 */
static inline hb_mc_epa_t hb_mc_npa_get_epa(const hb_mc_npa_t *npa)
{
    return npa->epa;
}


/**
 * Set the X coordinate of #npa.
 * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
 * @param[in] X     The new X coordinate.
 * @return A pointer to the modified #npa.
 */
static inline hb_mc_npa_t* hb_mc_npa_set_x(hb_mc_npa_t *npa, hb_mc_idx_t x)
{
    npa->x = x;
    return npa;
}

/**
 * Set the Y coordinate of #npa.
 * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
 * @param[in] y     The new Y coordinate.
 * @return A pointer to the modified #npa.
 */
static inline hb_mc_npa_t* hb_mc_npa_set_y(hb_mc_npa_t *npa, hb_mc_idx_t y)
{
    npa->y = y;
    return npa;
}


/**
 * Set the Endpoint Physical Address of #npa.
 * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
 * @param[in] epa   An Endpoint Physical Address.
 * @return A pointer to the modified #npa.
 */
static inline hb_mc_npa_t* hb_mc_npa_set_epa(hb_mc_npa_t *npa, hb_mc_epa_t epa)
{
    npa->epa = epa;
    return npa;
}

/**
 * Create a Network Physical Address from an X and Y coordinate and an EPA.
 * @param[in] x    X coordinate.
 * @param[in] y    Y coordinate.
 * @param[in] epa  An Endpoint Physical Address.
 * @return A Network Physical Address.
 */
static inline hb_mc_npa_t hb_mc_npa_from_x_y(hb_mc_idx_t x, hb_mc_idx_t y, hb_mc_epa_t epa)
{
    hb_mc_npa_t npa;

    hb_mc_npa_set_x(&npa, x);
    hb_mc_npa_set_y(&npa, y);
    hb_mc_npa_set_epa(&npa, epa);
    
    return npa;
}

/**
 * Create a Network Physical Address from a coordinate and an EPA.
 * @param[in] c    A coordinate.
 * @param[in] epa  An Endpoint Physical Address.
 * @return A Network Physical Address.
 */
static inline hb_mc_npa_t hb_mc_npa(hb_mc_coordinate_t c, hb_mc_epa_t epa)
{
    return hb_mc_npa_from_x_y(hb_mc_coordinate_get_x(c),
			      hb_mc_coordinate_get_y(c),
			      epa);    
}



#ifdef __cplusplus
};
#endif
#endif
