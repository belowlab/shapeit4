////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2018 Olivier Delaneau, University of Lausanne
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////


//  h0 000 / h1 100 / h2 010 / h3 110 / h4 001 / h5 101 / h6 011 / h7 111	//haplotype ordering
// -------- -------- -------- -------- -------- -------- -------- --------
// 77777777 66666666 55555555 44444444 33333333 22222222 11111111 00000000	//haplotype h0
// 76543210 76543210 76543210 76543210 76543210 76543210 76543210 76543210	//haplotype h1
// -------- -------- -------- -------- -------- -------- -------- --------
// 11111111 11111111 11111111 11111111111111111 11111111 11111111 11111111	//mask for initial.		0xFFFFFFFFFFFFFFFFUL
// 00000000 10101010 00000000 10101010 00000000 10101010 00000000 10101010	//mask for scaffold		0x00AA00AA00AA00AAUL
// 01010101 10101010 01010101 10101010 01010101 10101010 01010101 10101010	//mask unfold = 0 		0x55AA55AA55AA55AAUL
// 00110011 00110011 11001100 11001100 00110011 00110011 11001100 11001100	//mask unfold = 1 		0x3333CCCC3333CCCCUL
// 00001111 00001111 00001111 00001111 11110000 11110000 11110000 11110000	//mask unfold = 2 		0x0F0F0F0FF0F0F0F0UL

#define MASK_INIT	0xFFFFFFFFFFFFFFFFUL
#define MASK_SCAF	0x00AA00AA00AA00AAUL
#define MASK_UNF0	0x55AA55AA55AA55AAUL
#define MASK_UNF1	0x3333CCCC3333CCCCUL
#define MASK_UNF2	0x0F0F0F0FF0F0F0F0UL

#include <objects/genotype/genotype_header.h>

void genotype::build() {
	//1. Count number of segments
	unsigned n_unf = 0, n_var = 0, n_sca = 0, n_seg = 0, n_amb = 0;
	for (unsigned int v = 0 ; v < n_variants ;) {
		bool f_sca = VAR_GET_SCA(MOD2(v),Variants[DIV2(v)]);
		bool f_het = VAR_GET_HET(MOD2(v),Variants[DIV2(v)]);
		bool f_mis = VAR_GET_MIS(MOD2(v),Variants[DIV2(v)]);

		unsigned int predicted_unfold = n_unf + (f_het||f_mis) + (n_sca||f_sca);
		if (predicted_unfold == 4 || (n_var == std::numeric_limits< unsigned short >::max())) {
			n_unf = 0;
			n_sca = 0;
			n_var = 0;
			n_seg ++;
		} else {
			n_unf += (f_het||f_mis);
			n_sca += f_sca;
			n_amb += (f_het||f_mis||f_sca);
			n_var ++;
			v++;
		}
	}
	n_segments = n_seg + 1;
	n_ambiguous = n_amb;

	//2. Build Segments
	n_unf = 0, n_var = 0, n_sca = 0, n_seg = 0, n_amb = 0;
	Lengths = vector < unsigned short > (n_segments, 0U);
	for (unsigned int v = 0 ; v < n_variants ;) {
		bool f_sca = VAR_GET_SCA(MOD2(v),Variants[DIV2(v)]);
		bool f_het = VAR_GET_HET(MOD2(v),Variants[DIV2(v)]);
		bool f_mis = VAR_GET_MIS(MOD2(v),Variants[DIV2(v)]);

		unsigned int predicted_unfold = n_unf + (f_het||f_mis) + (n_sca||f_sca);
		if (predicted_unfold == 4 || (n_var == std::numeric_limits< unsigned short >::max())) {
			Lengths[n_seg] = n_var;
			n_unf = 0;
			n_sca = 0;
			n_var = 0;
			n_seg ++;
		} else {
			n_unf += (f_het||f_mis);
			n_sca += f_sca;
			n_amb += (f_het||f_mis||f_sca);
			n_var ++;
			v++;
		}
	}
	Lengths[n_seg] = n_var;

	//3. Build Ambiguous
	Ambiguous = vector < unsigned char >(n_ambiguous, 0U);
	vector < unsigned char > orderedSegments = vector < unsigned char >(n_segments, 0);
	for (unsigned int s = 0, a0 = 0, a1 = 0, a2 = 0, vabs = 0 ; s < n_segments ; s ++) {
		for (unsigned int vrel = 0 ; vrel < Lengths[s] ; vrel ++) {
			bool f_sca = VAR_GET_SCA(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
			bool f_het = VAR_GET_HET(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
			bool f_mis = VAR_GET_MIS(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
			if (f_sca) {
				for (unsigned int h = 0 ; h < HAP_NUMBER ; h ++) {
					bool allele = (h%2)?VAR_GET_HAP1(MOD2(vabs+vrel), Variants[DIV2(vabs+vrel)]):VAR_GET_HAP0(MOD2(vabs+vrel), Variants[DIV2(vabs+vrel)]);
					if (allele) HAP_SET(Ambiguous[a0], h);
				}
				orderedSegments[s] = 1;
			}
			a0 += (f_sca + f_het + f_mis);
		}
		unsigned int n_unf = orderedSegments[s];
		for (unsigned int vrel = 0 ; vrel < Lengths[s] ; vrel ++) {
			bool f_sca = VAR_GET_SCA(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
			bool f_het = VAR_GET_HET(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
			bool f_mis = VAR_GET_MIS(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
			if (f_het||f_mis) {
				for (unsigned int h = 0 ; h < HAP_NUMBER ; h ++) {
					bool allele = ((h>>n_unf)%2);
					if (allele) HAP_SET(Ambiguous[a1], h);
				}
				n_unf++;
			}
			a1 += (f_sca + f_het + f_mis);
		}
		vabs += Lengths[s];
	}

	//4. Build Diplotypes
	Diplotypes = vector < unsigned long > (n_segments);
	for (unsigned int s = 0, vabs = 0, a = 0 ; s < n_segments ; s ++) {
		unsigned int n_unf = orderedSegments[s];
		Diplotypes[s]=n_unf?MASK_SCAF:MASK_INIT;
		for (unsigned int vrel = 0 ; vrel < Lengths[s] ; vrel ++) {
			bool f_het = VAR_GET_HET(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
			bool f_mis = VAR_GET_MIS(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
			if (f_het) {
				switch (n_unf) {
				case 0: Diplotypes[s] &= MASK_UNF0; break;
				case 1: Diplotypes[s] &= MASK_UNF1; break;
				case 2: Diplotypes[s] &= MASK_UNF2; break;
				}
			}
			n_unf += (f_het||f_mis);
		}
		for (unsigned int vrel = 0 ; vrel < Lengths[s] ; vrel ++) a+=VAR_GET_AMB(MOD2(vabs+vrel),Variants[DIV2(vabs+vrel)]);
		vabs += Lengths[s];
	}

	//5. Count transitions
	n_transitions = countTransitions();
}
