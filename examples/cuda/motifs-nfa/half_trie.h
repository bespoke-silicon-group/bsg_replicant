// Copyright (C) 2010  
// Pierluigi Rolando (pierluigi.rolando@polito.it)
// Netgroup - DAUIN - Politecnico di Torino
//
// Niccolo' Cascarano (niccolo.cascarano@polito.it)
// Netgroup - DAUIN - Politecnico di Torino
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef __HALF_TRIE_H__
#define __HALF_TRIE_H__

#include "utils.h"

#include <iostream>
#include <cassert>
#include <ext/hash_map>

namespace std { using namespace __gnu_cxx; }
using namespace utils;

namespace half_trie {

template<typename in_sym, typename out_sym>
class HalfTrie {
	private:
		enum {
			maps_size = 0xFFFF,
		};

		out_sym max_out_;
		in_sym  max_in_;
		std::hash_map<in_sym, out_sym> *maps[maps_size];

	public:
		~HalfTrie() {
			for(unsigned i = 0; i < 0xFFFF; ++i)
				delete maps[i];
			return;
		}

		out_sym get_max_out() const {
			return max_out_;
		}

		HalfTrie(std::ifstream &in, out_sym &max_out) : 
			max_out_(0), max_in_(0) {
			
			for(unsigned i = 0; i < maps_size; ++i)
				maps[i] = 0;

			in_sym start, end;
			out_sym out;

			unsigned lines = 0;
			while(read_line(in, start, end, out)) {
				++lines;
				if(out > max_out)
					max_out = out;
				if(end > max_in_)
					max_in_ = end;
				while(start <= end)
					put(start++, out);
			}

			std::clog << "lines: " << lines << '\n';

			assert(max_out);
		}


		void put(in_sym in, out_sym out) {
			std::hash_map<in_sym, out_sym> *hm(0);
			if(!(hm = maps[(in >> 16) & 0xFFFF])) {
				maps[(in >> 16) & 0xFFFF] = hm = new std::hash_map<in_sym,
					out_sym>();
				assert(hm);
			}

			(*hm)[in & 0xFFFF] = out;
			return;
		}

		out_sym get(in_sym in, out_sym max_out) const {
			typename std::hash_map<in_sym, out_sym>::iterator i;
			std::hash_map<in_sym, out_sym> *hm(maps[(in >> 16) & 0xFFFF]);

			if(!hm || (i = hm->find(in & 0xFFFF)) == hm->end())
				return max_out;

			return i->second;
		}

		template<typename T>
			unsigned translate(const std::vector<T> in, std::vector<out_sym> &out,
					size_t length) const {
#ifdef DEBUG
				std::cout << "Translating..."  << std::endl;
				std::cout << "Length in: " << in.size()
					<< " Size: " << length << std::endl;
#endif
				for(unsigned i = 0; i < (length>>1)<<1; i += 2) {
					in_sym current = (in[i] << 16) | (in[i+1]);
#ifdef DEBUG
					using namespace std;
					cout << "Symbol " << i/2 << ": ";
					cout << hex << current << dec << " <-> " << current << endl;
#endif
					out.push_back(get(current, max_out_+1));
				}

				// XXX: add padding 'cause we need 2 input symbols at a time.
				// TODO: this must be fixed by adding a non-existing symbol to the input
				// alphabet, then using it here. The automaton graph must be modified
				// accordingly by making final states loop on the additional symbol as
				// well.
				if(length%2) {
					in_sym current = (in[length-1] << 16) | (max_in_ & 0xFFFF);
#ifdef DEBUG
					using namespace std;
					cout << "Symbol " << length/2 << ": ";
					cout << hex << current << dec << " <-> " << current 
						<< " (padded)" <<endl;

#endif
					out.push_back(get(current, max_out_+1));
				}

				return out.size();
			}
};

} // namespace half_trie

#endif /* __HALF_TRIE_H__ */

