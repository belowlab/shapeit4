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
#include <containers/genotype_set.h>

genotype_set::genotype_set() {
	n_site = 0;
	n_ind = 0;
}

genotype_set::~genotype_set() {
	for (int i = 0 ; i< vecG.size() ; i ++) delete vecG[i];
	vecG.clear();
	n_site = 0;
	n_ind = 0;
}

void genotype_set::imputeMonomorphic(variant_map & V) {
	for (unsigned int v = 0 ; v < V.size() ; v ++) {
		if (V.vec_pos[v]->isMonomorphic()) {
			bool uallele = (V.vec_pos[v]->cref)?false:true;
			for (unsigned int i = 0 ; i < vecG.size() ; i ++) {
				VAR_SET_HOM(MOD2(v), vecG[i]->Variants[DIV2(v)]);
				uallele?VAR_SET_HAP0(MOD2(v), vecG[i]->Variants[DIV2(v)]):VAR_CLR_HAP0(MOD2(v), vecG[i]->Variants[DIV2(v)]);
				uallele?VAR_SET_HAP1(MOD2(v), vecG[i]->Variants[DIV2(v)]):VAR_CLR_HAP1(MOD2(v), vecG[i]->Variants[DIV2(v)]);
			}
			if (uallele) V.vec_pos[v]->cref = 0;
			else V.vec_pos[v]->calt = 0;
			V.vec_pos[v]->cmis = 0;
		}
	}
}

unsigned int genotype_set::largestNumberOfTransitions() {
	unsigned int maxT = 0;
	for (int i = 0 ; i < n_ind ; i ++) {
		unsigned int nTrans = vecG[i]->n_transitions;
		if (nTrans > maxT) maxT = nTrans;
	}
	return maxT;
}

unsigned int genotype_set::largestNumberOfMissings() {
	unsigned int maxM = 0;
	for (int i = 0 ; i < n_ind ; i ++) {
		unsigned int nMis = vecG[i]->n_missing * HAP_NUMBER;
		if (nMis> maxM) maxM = nMis;
	}
	return maxM;
}

unsigned long genotype_set::numberOfSegments() {
	unsigned long size = 0;
	for (int i = 0 ; i < n_ind ; i ++) size += vecG[i]->n_segments;
	return size;
}

void genotype_set::masking() {
	for (int i = 0 ; i < n_ind ; i ++) vecG[i]->mask();
}

void genotype_set::solve() {
	tac.clock();
	for (int i = 0 ; i < vecG.size() ; i ++) vecG[i]->solve();
	vrb.bullet("HAP solving (" + stb.str(tac.rel_time()*1.0/1000, 2) + "s)");
}
