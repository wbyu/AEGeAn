#include <omp.h>
#include "AgnLocus.h"
#include "AgnLocusIndex.h"
#include "AgnPairwiseCompareLocus.h"
#include "AgnUtils.h"

//------------------------------------------------------------------------------
// Data structure definition
//------------------------------------------------------------------------------
struct AgnLocusIndex
{
  GtStrArray *seqids;
  GtHashmap *locus_trees;
};


//------------------------------------------------------------------------------
// Prototypes for private methods
//------------------------------------------------------------------------------

/**
 * Given two sets of annotations for the same sequence (a reference set and a
 * prediction set), this function associates each gene annotation with the
 * appropriate locus.
 *
 * @param[in] idx       the locus index
 * @param[in] seqid     the sequence for which loci are requested
 * @param[in] refr      reference annotations for the sequence
 * @param[in] pred      prediction annotations for the sequence
 * @param[in] logger    object to which warning/error messages are written
 * @returns             an interval tree containing the sequence's gene loci
 */
GtIntervalTree *agn_locus_index_parse_pairwise(AgnLocusIndex *idx,
                                               const char *seqid,
                                               GtFeatureIndex *refr,
                                               GtFeatureIndex *pred,
                                               AgnLogger *logger);

/**
 * Given a locus and a collection of gene features, search for genes that
 * overlap with the locus and assign them to the locus.
 *
 * @param[in]  idx              locus index object
 * @param[in]  features         collection of gene features
 * @param[in]  visited_genes    list of genes that have already been assigned to
 *                              a locus and which should be ignored
 * @param[out] locus            the locus of interest, to which overlapping
 *                              genes will be assigned
 * @param[in]  add_func         the function that should be used to add genes to
 *                              the locus (distinguishes between reference and
 *                              prediction)
 * @param[in]  logger           object to which warning/error messages are
 *                              written
 * @returns                     the number of genes assigned to the locus
 */
int agn_locus_index_pairwise_test_overlap(AgnLocusIndex *idx,
                  GtFeatureIndex *features, GtHashmap *visited_genes,
                  AgnPairwiseCompareLocus *locus,
                  void (*add_func)(AgnPairwiseCompareLocus *, GtFeatureNode *),
                  AgnLogger *logger);

/**
 * Given a locus and a collection of genomic features, search for genes that
 * overlap with the locus and assign them to the locus.
 *
 * @param[in]  idx              locus index object
 * @param[in]  features         collection of genomic features
 * @param[in]  visited_genes    list of genes that have already been assigned to
 *                              a locus and which should be ignored
 * @param[out] locus            the locus of interest, to which overlapping
 *                              genes will be assigned
 * @param[in]  logger           object to which warning/error messages are
 *                              written
 * @returns                     the number of genes assigned to the locus
 */
int agn_locus_index_test_overlap(AgnLocusIndex *idx, GtFeatureIndex *features,
                                 GtHashmap *visited_genes, AgnLocus *locus,
                                 AgnLogger *logger);


//------------------------------------------------------------------------------
// Method implementations
//------------------------------------------------------------------------------

void agn_locus_index_delete(AgnLocusIndex *idx)
{
  gt_hashmap_delete(idx->locus_trees);
  gt_str_array_delete(idx->seqids);
  gt_free(idx);
  idx = NULL;
}

AgnLocusIndex *agn_locus_index_new()
{
  AgnLocusIndex *idx = gt_malloc( sizeof(AgnLocusIndex *) );
  idx->seqids = gt_str_array_new();
  idx->locus_trees = gt_hashmap_new(GT_HASH_STRING, NULL,
                                    (GtFree)gt_interval_tree_delete);
  return idx;
}

unsigned long agn_locus_index_parse_disk(AgnLocusIndex * idx, int numfiles,
                  const char **filenames, int numprocs, AgnLogger *logger)
{
  gt_assert(idx != NULL);
  unsigned long nloci;
  // Do I want to use the import canonical function or the add_gff3 function?
  GtFeatureIndex *features = agn_import_canonical(numfiles, filenames, logger);
  if(agn_logger_has_error(logger))
  {
    gt_feature_index_delete(features);
    return 0;
  }
  
  nloci = agn_locus_index_parse_memory(idx, features, numprocs, logger);
  gt_feature_index_delete(features);
  return nloci;
}

int agn_locus_index_pairwise_test_overlap(AgnLocusIndex *idx,
                  GtFeatureIndex *features, GtHashmap *visited_genes,
                  AgnPairwiseCompareLocus *locus,
                  void (*add_func)(AgnPairwiseCompareLocus *, GtFeatureNode *),
                  AgnLogger *logger)
{
  GtRange locusrange;
  GtError *error = gt_error_new();
  GtArray *genes_to_add = gt_array_new( sizeof(GtFeatureNode *) );
  unsigned long new_gene_count;
  
  locusrange.start = agn_pairwise_compare_locus_get_start(locus);
  locusrange.end = agn_pairwise_compare_locus_get_end(locus);
  gt_feature_index_get_features_for_range(features, genes_to_add,
          agn_pairwise_compare_locus_get_seqid(locus), &locusrange, error);
  if(gt_error_is_set(error))
  {
    agn_logger_log_error(logger, "error fetching features for range %s[%lu, "
                         "%lu]: %s",agn_pairwise_compare_locus_get_seqid(locus),
                         locusrange.start, locusrange.end, gt_error_get(error));
    gt_error_unset(error);
  }
  
  while(gt_array_size(genes_to_add) > 0)
  {
    GtFeatureNode *gene_to_add = *(GtFeatureNode **)gt_array_pop(genes_to_add);
    if(gt_hashmap_get(visited_genes, gene_to_add) == NULL)
    {
      gt_hashmap_add(visited_genes, gene_to_add, gene_to_add);
      add_func(locus, gene_to_add);
      new_gene_count++;
    }
  }
  
  gt_array_delete(genes_to_add);
  gt_error_delete(error);
  
  return new_gene_count;
}

GtIntervalTree *agn_locus_index_parse(AgnLocusIndex *idx, const char *seqid,
                                      GtFeatureIndex *features,
                                      AgnLogger *logger)
{
  unsigned long i;
  GtError *error = gt_error_new();
  GtHashmap *visited_genes = gt_hashmap_new(GT_HASH_DIRECT, NULL, NULL);
  GtIntervalTree *loci = gt_interval_tree_new((GtFree)agn_locus_delete);
  
  GtArray *seqfeatures = gt_feature_index_get_features_for_seqid(features,
                                 seqid, error);
  if(gt_error_is_set(error))
  {
    agn_logger_log_error(logger, "error fetching features for sequence '%s': "
                         "%s", seqid, gt_error_get(error));
    gt_error_delete(error);
    gt_array_delete(seqfeatures);
    gt_hashmap_delete(visited_genes);
    gt_interval_tree_delete(loci);
    return NULL;
  }
  for(i = 0; i < gt_array_size(seqfeatures); i++)
  {
    GtFeatureNode *feature = *(GtFeatureNode **)gt_array_get(seqfeatures, i);
    GtFeatureNodeIterator *iter = gt_feature_node_iterator_new(feature);
    GtFeatureNode *fn;
    for(fn = gt_feature_node_iterator_next(iter);
        fn != NULL;
        fn = gt_feature_node_iterator_next(iter))
    {
      if(!gt_feature_node_has_type(fn, "gene") ||
         gt_hashmap_get(visited_genes, fn) != NULL)
      {
        continue;
      }
      
      AgnLocus *locus = agn_locus_new(seqid);
      agn_locus_add(locus, fn);
      
      int new_gene_count = 0;
      do
      {
        int temp_new_gene_count = agn_locus_index_test_overlap(idx,
                                      features, visited_genes, locus,
                                      logger);
        new_gene_count = temp_new_gene_count;
      } while(new_gene_count > 0);
      
      GtIntervalTreeNode *itn = gt_interval_tree_node_new(locus,
                                                          locus->range.start,
                                                          locus->range.end);
      gt_interval_tree_insert(loci, itn);
    }
  }
  
  gt_error_delete(error);
  gt_hashmap_delete(visited_genes);
  gt_array_delete(seqfeatures);
  return loci;
}

GtIntervalTree *agn_locus_index_parse_pairwise(AgnLocusIndex *idx,
                                               const char *seqid,
                                               GtFeatureIndex *refr,
                                               GtFeatureIndex *pred,
                                               AgnLogger *logger)
{
  unsigned long i;
  GtError *error = gt_error_new();
  GtHashmap *visited_genes = gt_hashmap_new(GT_HASH_DIRECT, NULL, NULL);
  GtIntervalTree *loci = gt_interval_tree_new(
                             (GtFree)agn_pairwise_compare_locus_delete);

  // Seed new loci with reference genes
  GtArray *refr_list = gt_feature_index_get_features_for_seqid(refr, seqid,
                                                               error);
  if(gt_error_is_set(error))
  {
    agn_logger_log_error(logger, "error fetching reference features for "
                         "sequence '%s': %s", seqid, gt_error_get(error));
    gt_error_delete(error);
    gt_array_delete(refr_list);
    gt_hashmap_delete(visited_genes);
    gt_interval_tree_delete(loci);
    return NULL;
  }
  for(i = 0; i < gt_array_size(refr_list); i++)
  {
    GtFeatureNode *refr_gene = *(GtFeatureNode**)gt_array_get(refr_list, i);
    if(gt_hashmap_get(visited_genes, refr_gene) != NULL)
      continue;
    
    gt_hashmap_add(visited_genes, refr_gene, refr_gene);
    AgnPairwiseCompareLocus *locus = agn_pairwise_compare_locus_new(seqid);
    agn_pairwise_compare_locus_add_refr_gene(locus, refr_gene);

    int new_gene_count = 0;
    do
    {
      int new_refr_gene_count = agn_locus_index_pairwise_test_overlap(idx, refr,
                                    visited_genes, locus,
                                    agn_pairwise_compare_locus_add_refr_gene,
                                    logger);
      int new_pred_gene_count = agn_locus_index_pairwise_test_overlap(idx, pred,
                                    visited_genes, locus,
                                    agn_pairwise_compare_locus_add_pred_gene,
                                    logger);
      if(agn_logger_has_error(logger))
      {
        gt_error_delete(error);
        gt_array_delete(refr_list);
        gt_hashmap_delete(visited_genes);
        gt_interval_tree_delete(loci);
        return NULL;
      }
      new_gene_count = new_refr_gene_count + new_pred_gene_count;
    } while(new_gene_count > 0);
    GtIntervalTreeNode *itn = gt_interval_tree_node_new(locus,
                                  agn_pairwise_compare_locus_get_start(locus),
                                  agn_pairwise_compare_locus_get_end(locus));
    gt_interval_tree_insert(loci, itn);
  }
  gt_array_delete(refr_list);

  // All reference genes, and some prediction genes, have been assigned to loci.
  // Now seed new loci with prediction genes, ignoring reference genes.
  GtArray *pred_list = gt_feature_index_get_features_for_seqid(pred, seqid,
                                                               error);
  if(gt_error_is_set(error))
  {
    agn_logger_log_error(logger, "error fetching prediction features for "
                         "sequence '%s': %s", seqid, gt_error_get(error));
    gt_error_delete(error);
    gt_array_delete(refr_list);
    gt_hashmap_delete(visited_genes);
    gt_interval_tree_delete(loci);
    return NULL;
  }
  for(i = 0; i < gt_array_size(pred_list); i++)
  {
    GtFeatureNode *pred_gene = *(GtFeatureNode**)gt_array_get(pred_list,i);
    if(gt_hashmap_get(visited_genes, pred_gene) != NULL)
      continue;
    
    gt_hashmap_add(visited_genes, pred_gene, pred_gene);
    AgnPairwiseCompareLocus *locus = agn_pairwise_compare_locus_new(seqid);
    agn_pairwise_compare_locus_add_pred_gene(locus, pred_gene);
    
    int new_gene_count = 0;
    do
    {
      int new_pred_gene_count = agn_locus_index_pairwise_test_overlap(idx, pred,
                                    visited_genes, locus,
                                    agn_pairwise_compare_locus_add_pred_gene,
                                    logger);
      if(agn_logger_has_error(logger))
      {
        gt_error_delete(error);
        gt_array_delete(refr_list);
        gt_hashmap_delete(visited_genes);
        gt_interval_tree_delete(loci);
        return NULL;
      }
      new_gene_count = new_pred_gene_count;
    } while(new_gene_count > 0);
    
    GtIntervalTreeNode *itn = gt_interval_tree_node_new(locus,
                                  agn_pairwise_compare_locus_get_start(locus),
                                  agn_pairwise_compare_locus_get_end(locus));
    gt_interval_tree_insert(loci, itn);
  }
  
  gt_error_delete(error);
  gt_hashmap_delete(visited_genes);
  gt_array_delete(pred_list);

  return loci;
}

unsigned long agn_locus_index_parse_pairwise_memory(AgnLocusIndex *idx,
                  GtFeatureIndex *refrfeats, GtFeatureIndex *predfeats,
                  int numprocs, AgnLogger *logger)
{
  int i, rank;
  gt_assert(idx != NULL);
  gt_assert(refrfeats != NULL && predfeats != NULL);
  GtStrArray *seqids = agn_seq_intersection(refrfeats, predfeats, logger);
  if(agn_logger_has_error(logger))
  {
    gt_str_array_delete(seqids);
    return 0;
  }
  gt_str_array_delete(idx->seqids);
  idx->seqids = seqids;
  
  unsigned long totalloci = 0;
  #pragma omp parallel private(i, rank)
  {
    rank = omp_get_thread_num();

    #pragma omp for schedule(static)
    for(i = 0; i < gt_str_array_size(seqids); i++)
    {
      const char *seqid = gt_str_array_get(seqids, i);
      GtIntervalTree *loci = agn_locus_index_parse_pairwise(idx, seqid,
                                                            refrfeats,
                                                            predfeats, logger);
      #pragma omp critical
      {
        totalloci += gt_interval_tree_size(loci);
        gt_hashmap_add(idx->locus_trees, (char *)seqid, loci);
        agn_logger_log_status(logger, "loci for sequence '%s' identified by "
                              "processor %d", seqid, rank);
      }
    }
  } // End parallelize
  
  return totalloci;
}

unsigned long agn_locus_index_parse_pairwise_disk(AgnLocusIndex *idx,
                  const char *refrfile, const char *predfile, int numprocs,
                  AgnLogger *logger)
{
  gt_assert(idx != NULL);
  unsigned long nloci;
  GtFeatureIndex *refrfeats = agn_import_canonical(1, &refrfile, logger);
  GtFeatureIndex *predfeats = agn_import_canonical(1, &predfile, logger);
  if(agn_logger_has_error(logger))
  {
    gt_feature_index_delete(refrfeats);
    gt_feature_index_delete(predfeats);
    return 0;
  }
  
  nloci = agn_locus_index_parse_pairwise_memory(idx, refrfeats, predfeats,
                                                numprocs, logger);
  gt_feature_index_delete(refrfeats);
  gt_feature_index_delete(predfeats);
  return nloci;
}

unsigned long agn_locus_index_parse_memory(AgnLocusIndex * idx,
                  GtFeatureIndex *features, int numprocs, AgnLogger *logger)
{
  int i, rank;
  gt_assert(idx != NULL && features != NULL);
  GtError *error = gt_error_new();
  GtStrArray *seqids = gt_feature_index_get_seqids(features, error);
  if(gt_error_is_set(error))
  {
    agn_logger_log_error(logger, "error fetching seqids: %s",
                         gt_error_get(error));
    gt_error_delete(error);
    gt_str_array_delete(seqids);
    return 0;
  }
  gt_str_array_delete(idx->seqids);
  idx->seqids = seqids;
  
  unsigned long totalloci = 0;
  #pragma omp parallel private(i, rank)
  {
    rank = omp_get_thread_num();
    
    #pragma omp for schedule(static)
    for(i = 0; i < gt_str_array_size(seqids); i++)
    {
      const char *seqid = gt_str_array_get(seqids, i);
      GtIntervalTree *loci = agn_locus_index_parse(idx,seqid,features,logger);
      #pragma omp critical
      {
        totalloci += gt_interval_tree_size(loci);
        gt_hashmap_add(idx->locus_trees, (char *)seqid, loci);
        agn_logger_log_status(logger, "loci for sequence '%s' identified by "
                              "processor %d", seqid, rank);
      }
    } // End parallelize
  }
  
  gt_error_delete(error);
  return totalloci;
}

int agn_locus_index_test_overlap(AgnLocusIndex *idx, GtFeatureIndex *features,
                                 GtHashmap *visited_genes, AgnLocus *locus,
                                 AgnLogger *logger)
{
  int new_gene_count = 0;
  GtError *error = gt_error_new();
  GtArray *overlapping_features = gt_array_new( sizeof(GtFeatureNode *) );
  
  gt_feature_index_get_features_for_range(features, overlapping_features,
                                          locus->seqid, &locus->range, error);
  if(gt_error_is_set(error))
  {
    agn_logger_log_error(logger, "error fetching features for range "
                         "%s[%lu, %lu]: %s", locus->seqid,
                         locus->range.start, locus->range.end,
                         gt_error_get(error));
    gt_error_delete(error);
    gt_array_delete(overlapping_features);
    return 0;
  }
  
  while(gt_array_size(overlapping_features) > 0)
  {
    GtFeatureNode *fn = *(GtFeatureNode **)gt_array_pop(overlapping_features);
    if(gt_feature_node_has_type(fn, "gene") &&
       gt_hashmap_get(visited_genes, fn) == NULL)
    {
      gt_hashmap_add(visited_genes, fn, fn);
      agn_locus_add(locus, fn);
      new_gene_count++;
    }
  }
  gt_array_delete(overlapping_features);
  
  return new_gene_count;
}