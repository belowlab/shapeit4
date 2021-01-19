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
#include <phaser/phaser_header.h>

#include <io/haplotype_writer.h>

void * phaseWindow_callback(void * ptr) {
	phaser * S = static_cast< phaser * >( ptr );
	int id_worker, id_job;
	pthread_mutex_lock(&S->mutex_workers);
	id_worker = S->i_workers ++;
	pthread_mutex_unlock(&S->mutex_workers);
	for(;;) {
		pthread_mutex_lock(&S->mutex_workers);
		id_job = S->i_jobs ++;
		if (id_job <= S->G.n_ind) vrb.progress("  * HMM computations", id_job*1.0/S->G.n_ind);
		pthread_mutex_unlock(&S->mutex_workers);
		if (id_job < S->G.n_ind) S->phaseWindow(id_worker, id_job);
		else pthread_exit(NULL);
	}
}

void phaser::phaseWindow(int id_worker, int id_job) {
	threadData[id_worker].make(id_job, options["window"].as < double > ());
	for (int w = 0 ; w < threadData[id_worker].size() ; w ++) {
		if (options["thread"].as < int > () > 1) pthread_mutex_lock(&mutex_workers);
		statH.push(threadData[id_worker].Kvec[w].size()*1.0);
		statS.push((V.vec_pos[threadData[id_worker].C[w].stop_locus]->bp - V.vec_pos[threadData[id_worker].C[w].start_locus]->bp + 1) * 1.0 / 1e6);
		if (options["thread"].as < int > () > 1) pthread_mutex_unlock(&mutex_workers);

		if (threadData[id_worker].Kvec[w].size() == 0) {
			vrb.error("Could not find conditioning haplotypes for [" + G.vecG[id_job]->name  + "] / check options --pbwt-* and --ibd2-*");
		}

		int outcome = 0;
		if (G.vecG[id_job]->double_precision) {
			haplotype_segment_double HS(G.vecG[id_job], H.H_opt_hap, threadData[id_worker].Kvec[w], threadData[id_worker].C[w], M);
			HS.forward();
			outcome = HS.backward(threadData[id_worker].T, threadData[id_worker].M);
		} else {
			haplotype_segment_single HS(G.vecG[id_job], H.H_opt_hap, threadData[id_worker].Kvec[w], threadData[id_worker].C[w], M);
			HS.forward();
			outcome = HS.backward(threadData[id_worker].T, threadData[id_worker].M);
		}

		switch (outcome) {
		case -2: vrb.error("Diploid underflow impossible to recover for [" + G.vecG[id_job]->name + "]");
		case -1: vrb.error("Haploid underflow impossible to recover for [" + G.vecG[id_job]->name + "]");
		}
		n_underflow_recovered += outcome;
	}
	//Copy over IBD2 constraints into H
	if (options["thread"].as < int > () > 1) pthread_mutex_lock(&mutex_workers);
	for (int c = 0 ; c < threadData[id_worker].ind_ibd2.size() ; c++)
		H.bannedPairs[min(id_job, threadData[id_worker].ind_ibd2[c])].push_back(IBD2track(max(id_job, threadData[id_worker].ind_ibd2[c]), threadData[id_worker].start_ibd2[c], threadData[id_worker].end_ibd2[c]));
	if (options["thread"].as < int > () > 1) pthread_mutex_unlock(&mutex_workers);


	if (options.count("use-PS") && G.vecG[id_job]->ProbabilityMask.size() > 0) threadData[id_worker].maskingTransitions(id_job, options["use-PS"].as < double > ());

	vector < bool > flagMerges;
	switch (iteration_types[iteration_stage]) {
	case STAGE_BURN:	G.vecG[id_job]->sample(threadData[id_worker].T, threadData[id_worker].M);
						break;
	case STAGE_PRUN:	G.vecG[id_job]->sample(threadData[id_worker].T, threadData[id_worker].M);
						G.vecG[id_job]->mapMerges(threadData[id_worker].T, options["mcmc-prune"].as < double > (), flagMerges);
						G.vecG[id_job]->performMerges(threadData[id_worker].T, flagMerges);
						break;
	case STAGE_MAIN:	G.vecG[id_job]->sample(threadData[id_worker].T, threadData[id_worker].M);
						G.vecG[id_job]->store(threadData[id_worker].T, threadData[id_worker].M);
						break;
	}
}

void phaser::phaseWindow() {
	tac.clock();
	int n_thread = options["thread"].as < int > ();
	n_underflow_recovered = 0;
	i_workers = 0; i_jobs = 0;
	statH.clear(); statS.clear();
	storedKsizes.clear();
	if (n_thread > 1) {
		for (int t = 0 ; t < n_thread ; t++) pthread_create( &id_workers[t] , NULL, phaseWindow_callback, static_cast<void *>(this));
		for (int t = 0 ; t < n_thread ; t++) pthread_join( id_workers[t] , NULL);
	} else for (int i = 0 ; i < G.n_ind ; i ++) {
		phaseWindow(0, i);
		vrb.progress("  * HMM computations", (i+1)*1.0/G.n_ind);
	}
	if (n_underflow_recovered) vrb.bullet("HMM computations [K=" + stb.str(statH.mean(), 1) + "+/-" + stb.str(statH.sd(), 1) + " / W=" + stb.str(statS.mean(), 2) + "Mb / U=" + stb.str(n_underflow_recovered) + "] (" + stb.str(tac.rel_time()*1.0/1000, 2) + "s)");
	else vrb.bullet("HMM computations [K=" + stb.str(statH.mean(), 3) + "+/-" + stb.str(statH.sd(), 3) + " / W=" + stb.str(statS.mean(), 2) + "Mb] (" + stb.str(tac.rel_time()*1.0/1000, 2) + "s)");
}

void phaser::phase() {
	unsigned long n_old_segments = G.numberOfSegments(), n_new_segments = 0, current_iteration = 0;
	for (iteration_stage = 0 ; iteration_stage < iteration_counts.size() ; iteration_stage ++) {
		for (int iter = 0 ; iter < iteration_counts[iteration_stage] ; iter ++) {
			switch (iteration_types[iteration_stage]) {
			case STAGE_BURN:	vrb.title("Burn-in iteration [" + stb.str(iter+1) + "/" + stb.str(iteration_counts[iteration_stage]) + "]"); break;
			case STAGE_PRUN:	vrb.title("Pruning iteration [" + stb.str(iter+1) + "/" + stb.str(iteration_counts[iteration_stage]) + "]"); break;
			case STAGE_MAIN:	vrb.title("Main iteration [" + stb.str(iter+1) + "/" + stb.str(iteration_counts[iteration_stage]) + "]"); break;
			}
			H.transposeHaplotypes_V2H(false);
			//H.searchIBD2matching(G, V, min(V.lengthcM(), options["ibd2-length"].as < double > ()), options["window"].as < double > ()*0.5f, ibd2_maf, options["ibd2-mdr"].as < double > (), ibd2_count);
			H.updatePBWTmapping();
			H.selectPBWTarrays();
			H.transposePBWTarrays();
			phaseWindow();
			H.mergeIBD2constraints();
			H.updateHaplotypes(G);
			H.transposeHaplotypes_H2V(false);
			if (iteration_types[iteration_stage] == STAGE_PRUN) {
				n_new_segments = G.numberOfSegments();
				vrb.bullet("Trimming [pc=" + stb.str((1-n_new_segments*1.0/n_old_segments)*100, 2) + "%]");
				if (options.count("use-PS")) G.masking();
			}
		}
	}
}
