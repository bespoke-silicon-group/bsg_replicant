#pragma once
#include <infra_hb/host/device.hpp>
#include <infra_hb/host/global_scalar.hpp>
#include <set>
#include <map>
#include <sstream>
namespace hammerblade {
    typedef enum {
        BucketOrderIncreasing,
        BucketOrderDecreasing,
    } BucketOrder;

    typedef enum {
        PriorityOrderIncreasing,
        PriorityOrderDecreasing,
    } PriorityOrder;

    class Bucket {
    public:
        int              _id;
        std::set<int>    _identifiers;

        Bucket(){};
        bool empty() const { return _identifiers.empty(); }

        using Ptr = std::shared_ptr<Bucket>;
        template <typename ...Args>
        static std::shared_ptr<Bucket> Make(Args... args) {
            return std::shared_ptr<Bucket>(args...);
        }
    };

    template <typename T>
    class BucketPriorityQueue {
    private:
        int           _num_identifiers;
        GlobalScalar<hb_mc_eva_t> *_priority;
        BucketOrder   _bkt_order;
        PriorityOrder _pri_order;
        int           _total_buckets;
        T             _delta;
        std::map<int, int> _id_to_bucket;

        int           _current_bucket_idx;
        std::vector<Bucket> _buckets;
        T null_bkt = std::numeric_limits<T>::max();

    public:
        BucketPriorityQueue(int num_identifiers, GlobalScalar<hb_mc_eva_t> *priority,
                            BucketOrder bkt_order, PriorityOrder pri_order,
                            int total_buckets, T delta) :
            _num_identifiers(num_identifiers),
            _priority(priority),
            _bkt_order(bkt_order),
            _pri_order(pri_order),
            _total_buckets(total_buckets),
            _delta(delta),
            _current_bucket_idx(0),
            _buckets(total_buckets) {

            for (int bkt = 0; bkt < _buckets.size(); ++bkt) {
                _buckets[bkt]._id = bkt;
            }
            initBuckets();
        }

        bool currentBucketIsEmpty() const {
            return finished() || _buckets[_current_bucket_idx].empty();
        }

        void nextBucket() {
            if (_current_bucket_idx < _total_buckets)
                ++_current_bucket_idx;
        }

        int deltaPriorityToBucket(T delta_priority) const {
            int ret;
            if (_pri_order == PriorityOrderIncreasing) {
                ret = static_cast<int>(delta_priority);
            } else {
                ret = (_total_buckets-1) - static_cast<int>(delta_priority);
            }

            // should not happen
            if (ret >= _total_buckets || ret < 0) {
                std::stringstream ss;
                ss << "BucketPriorityQueue: error: total_buckets = " << _total_buckets;
                ss << ", calculated priority/delta = " << delta_priority;
                ss << ", maps to bucket " << ret;
                throw std::runtime_error(ss.str());
            }

            return ret;
        }

        Vector<int> popDenseReadyVertexSet() {
            // scan through buckets to find a none empty one
            while (!finished() && currentBucketIsEmpty()) {
                nextBucket();
            }

            // return empty vector if done
            if (finished())
                return Vector<int>(0);

            // create a dense set on the device
            std::vector<int> h_dense_o(_num_identifiers, 0);
            for (int id : _buckets[_current_bucket_idx]._identifiers) {
                h_dense_o[id] = 1;
                // remove from our bucket map because we're going to empy this
                // bucket in just a moment...
                _id_to_bucket.erase(id);
            }

            // empty this bucket
            _buckets[_current_bucket_idx]._identifiers.clear();

            Vector<int> d_dense_o(_num_identifiers);
            d_dense_o.copyToDevice(&h_dense_o[0], h_dense_o.size());

            // return the new dense set on the device
            return d_dense_o;
        }

        void initBuckets() {
            std::vector<T> h_priority(_num_identifiers);
            read_global_buffer<T>(&h_priority[0], *_priority, _num_identifiers);
            for(int id = 0; id < h_priority.size(); id++) {
                if(h_priority[id] == null_bkt)
                    continue;
                T delta_priority = h_priority[id]/_delta;
                int bkt_idx = deltaPriorityToBucket(delta_priority);

                // insert into mapped bucket
                Bucket &b = _buckets[bkt_idx];
                b._identifiers.insert(id);
                // mark this id as present
                _id_to_bucket[id] = bkt_idx;
            }
        }

        void updateWithDenseVertexSet(Vector<int> &d_dense_vertex_set) {
            // read the vertexset and priority the device
            std::vector<int> h_dense_vertex_set(_num_identifiers);
            d_dense_vertex_set.copyToHost(&h_dense_vertex_set[0], h_dense_vertex_set.size());

            std::vector<T> h_priority(_num_identifiers);
            read_global_buffer<T>(&h_priority[0], *_priority, _num_identifiers);

            // iteratate over each identifier and update its bucket
            for (int id = 0; id < h_dense_vertex_set.size(); ++id) {
                // dense set - skip if not in set
                if (!h_dense_vertex_set[id] || h_priority[id] == null_bkt)
                    continue;

                // calculate the delta-priority
                T delta_priority = h_priority[id]/_delta;

                // map this to a bucket
                int bkt_idx = deltaPriorityToBucket(delta_priority);

                // check to see if it is already in some other bucket
                auto it = _id_to_bucket.find(id);
                if (it != _id_to_bucket.end() &&
                    it->second != bkt_idx) {
                    // remove from other bucket
                    _buckets[it->second]._identifiers.erase(it->first);
                } else if (it != _id_to_bucket.end() &&
                           it->second  == bkt_idx) {
                    // we can just return - carry on
                    continue;
                }

                // insert into mapped bucket
                Bucket &b = _buckets[bkt_idx];
                b._identifiers.insert(id);
                // mark this id as present
                _id_to_bucket[id] = bkt_idx;
            }
        }

        bool finished() const {
            return _current_bucket_idx >= _total_buckets;
        }
    };
}
