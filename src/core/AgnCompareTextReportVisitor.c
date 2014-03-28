#include <math.h>
#include <stdarg.h>
#include "AgnCliquePair.h"
#include "AgnCompareTextReportVisitor.h"
#include "AgnLocus.h"
#include "AgnTranscriptClique.h"

#define compare_text_report_visitor_cast(GV)\
        gt_node_visitor_cast(compare_text_report_visitor_class(), GV)

//------------------------------------------------------------------------------
// Data structure definitions
//------------------------------------------------------------------------------

struct AgnCompareTextReportVisitor
{
  const GtNodeVisitor parent_instance;
  GtUword max_locus_transcripts;
  GtUword max_comparisons;
  bool gff3;
  FILE *fp_reports;
  FILE *fp_summary;
  GtLogger *logger;
};


//------------------------------------------------------------------------------
// Prototypes for private functions
//------------------------------------------------------------------------------

/**
 * @function Implement the interface to the GtNodeVisitor class.
 */
static const GtNodeVisitorClass *compare_text_report_visitor_class();

/**
 * @function For each locus, perform comparative analysis and report the
 * comparison statistics.
 */
static int ctrv_visit_feature_node(GtNodeVisitor *nv, GtFeatureNode *fn,
                                   GtError *error);

/**
 * @function FIXME
 */
static void locus_print_nucleotide_report(FILE *outstream,
                                          AgnCliquePair *pair);

/**
 * @function FIXME
 */
static void locus_print_structure_report(FILE *outstream,
                                         AgnCompStatsBinary *stats,
                                         const char *label, const char *units);

/**
 * @function FIXME
 */
static void locus_report_print_geneids(AgnCompareTextReportVisitor *v,
                                       AgnLocus *locus);

/**
 * @function FIXME
 */
static void locus_report_print_pair(AgnCompareTextReportVisitor *v,
                                    AgnCliquePair *pair);

/**
 * @function FIXME
 */
static void locus_report_print_unmatched_cliques(AgnCompareTextReportVisitor *v,
                                                 AgnLocus *locus);

/**
 * @function FIXME
 */
static void print_locus_report(AgnCompareTextReportVisitor *v, AgnLocus *locus);

/**
 * @function FIXME
 */
static void printf_c(FILE *outstream, const char *format, ...);


//------------------------------------------------------------------------------
// Method implementations
//------------------------------------------------------------------------------

void agn_compare_text_report_visitor_compare_max(AgnCompareTextReportVisitor *v,
                                                 GtUword max_comparisons)
{
  v->max_comparisons = max_comparisons;
}

void
agn_compare_text_report_visitor_enable_gff3(AgnCompareTextReportVisitor *v)
{
  v->gff3 = true;
}

GtNodeVisitor *agn_compare_text_report_visitor_new(FILE *reports,
                                                   FILE *summary,
                                                   GtLogger *logger)
{
  GtNodeVisitor*
    nv = gt_node_visitor_create(compare_text_report_visitor_class());
  AgnCompareTextReportVisitor *v = compare_text_report_visitor_cast(nv);
  v->max_locus_transcripts = 0;
  v->max_comparisons = 0;
  v->gff3 = false;
  v->fp_reports = reports;
  v->fp_summary = summary;
  v->logger = logger;
  return nv;
}

void agn_compare_text_report_visitor_trans_max(AgnCompareTextReportVisitor *v,
                                               GtUword max_locus_transcripts)
{
  v->max_locus_transcripts = max_locus_transcripts;
}

static const GtNodeVisitorClass *compare_text_report_visitor_class()
{
  static const GtNodeVisitorClass *nvc = NULL;
  if(!nvc)
  {
    nvc = gt_node_visitor_class_new(sizeof (AgnCompareTextReportVisitor), NULL,
                                    NULL, ctrv_visit_feature_node, NULL, NULL,
                                    NULL);
  }
  return nvc;
}

static int ctrv_visit_feature_node(GtNodeVisitor *nv, GtFeatureNode *fn,
                                   GtError *error)
{
  AgnCompareTextReportVisitor *v = compare_text_report_visitor_cast(nv);
  gt_error_check(error);

  gt_assert(gt_feature_node_has_type(fn, "locus"));
  AgnLocus *locus = (AgnLocus *)fn;
  agn_locus_comparative_analysis(locus, v->max_locus_transcripts,
                                 v->max_comparisons, v->logger);
  print_locus_report(v, locus);

  return 0;
}

static void locus_print_nucleotide_report(FILE *outstream,
                                          AgnCliquePair *pair)
{
  AgnComparison *pairstats = agn_clique_pair_get_stats(pair);
  double identity = (double)pairstats->overall_matches /
                    (double)pairstats->overall_length;
  double tolerance = agn_clique_pair_tolerance(pair);
  if(fabs(identity - 1.0) < tolerance)
    printf_c(outstream, "     |    Gene structures match perfectly!\n");
  else
  {
    printf_c(outstream, "     |    %-30s %-10s %-10s %-10s\n",
             "Nucleotide-level comparison", "CDS", "UTRs", "Overall" );
    printf_c(outstream, "     |    %-30s %-10s %-10s %.3lf\n",
             "Matching coefficient:", pairstats->cds_nuc_stats.mcs,
             pairstats->utr_nuc_stats.mcs, identity);
    printf_c(outstream, "     |    %-30s %-10s %-10s %-10s\n",
             "Correlation coefficient:", pairstats->cds_nuc_stats.ccs,
             pairstats->utr_nuc_stats.ccs, "--");
    printf_c(outstream, "     |    %-30s %-10s %-10s %-10s\n", "Sensitivity:",
             pairstats->cds_nuc_stats.sns, pairstats->utr_nuc_stats.sns, "--");
    printf_c(outstream, "     |    %-30s %-10s %-10s %-10s\n", "Specificity:",
             pairstats->cds_nuc_stats.sps, pairstats->utr_nuc_stats.sps, "--");
    printf_c(outstream, "     |    %-30s %-10s %-10s %-10s\n", "F1 Score:",
             pairstats->cds_nuc_stats.f1s, pairstats->utr_nuc_stats.f1s, "--");
    printf_c(outstream, "     |    %-30s %-10s %-10s %-10s\n",
             "Annotation edit distance:", pairstats->cds_nuc_stats.eds,
             pairstats->utr_nuc_stats.eds, "--");
  }

  printf_c(outstream, "     |\n");
}

static void locus_print_structure_report(FILE *outstream,
                                         AgnCompStatsBinary *stats,
                                         const char *label, const char *units)
{
  printf_c(outstream, " | %s structure comparison\n", label);
  if(stats->missing == 0 && stats->wrong == 0)
  {
    printf_c(outstream,
            "     |    %lu reference  %s\n"
            "     |    %lu prediction %s\n"
            "     |    %s structures match perfectly!\n",
            stats->correct, units, stats->correct, units, label);
  }
  else
  {
    printf_c(outstream,
            "     |    %lu reference %s\n"
            "     |        %lu match prediction\n"
            "     |        %lu don't match prediction\n"
            "     |    %lu prediction %s\n"
            "     |        %lu match reference\n"
            "     |        %lu don't match reference\n",
             stats->correct + stats->missing, units,
             stats->correct, stats->missing,
             stats->correct + stats->wrong, units,
             stats->correct, stats->wrong);
    printf_c(outstream,
            "     |    %-30s %-10s\n" 
            "     |    %-30s %-10s\n" 
            "     |    %-30s %-10s\n" 
            "     |    %-30s %-10s\n",
            "Sensitivity:", "Specificity:", "F1 Score:",
            "Annotation edit distance:", stats->sns, stats->sps, stats->f1s,
            stats->eds);
  }
  printf_c(outstream, "     |\n");
}

static void locus_report_print_geneids(AgnCompareTextReportVisitor *v,
                                       AgnLocus *locus)
{
  GtArray *genes;

  genes = agn_locus_refr_gene_ids(locus);
  printf_c(v->fp_reports, "  |  reference genes:\n");
  if(gt_array_size(genes) == 0)
    printf_c(v->fp_reports, "|    None!\n");
  while(gt_array_size(genes) > 0)
  {
    const char **geneid = gt_array_pop(genes);
    printf_c(v->fp_reports, "|    %s\n", *geneid);
  }
  printf_c(v->fp_reports, "|\n");
  gt_array_delete(genes);

  genes = agn_locus_pred_gene_ids(locus);
  printf_c(v->fp_reports, "  |  prediction genes:\n");
  if(gt_array_size(genes) == 0)
    printf_c(v->fp_reports, "|    None!\n");
  while(gt_array_size(genes) > 0)
  {
    const char **geneid = gt_array_pop(genes);
    printf_c(v->fp_reports, "|    %s\n", *geneid);
  }
  printf_c(v->fp_reports, "|\n");
  gt_array_delete(genes);
}

static void locus_report_print_pair(AgnCompareTextReportVisitor *v,
                                    AgnCliquePair *pair)
{
  GtArray *tids;
  AgnTranscriptClique *refrclique, *predclique;

  refrclique = agn_clique_pair_get_refr_clique(pair);
  tids = agn_transcript_clique_ids(refrclique);
  printf_c(v->fp_reports, "     |  reference transcripts:\n");
  while(gt_array_size(tids) > 0)
  {
    const char **tid = gt_array_pop(tids);
    printf_c(v->fp_reports, "     |    %s\n", *tid);
  }
  gt_array_delete(tids);

  predclique = agn_clique_pair_get_pred_clique(pair);
  tids = agn_transcript_clique_ids(predclique);
  printf_c(v->fp_reports, "     |  prediction transcripts:\n");
  while(gt_array_size(tids) > 0)
  {
    const char **tid = gt_array_pop(tids);
    printf_c(v->fp_reports, "     |    %s\n", *tid);
  }
  gt_array_delete(tids);

  printf_c(v->fp_reports, "     |\n");

  if(v->gff3)
  {
    printf_c(v->fp_reports, " | reference GFF3:\n");
    agn_transcript_clique_to_gff3(refrclique, v->fp_reports, " | ");
    printf_c(v->fp_reports, " | prediction GFF3:\n");
    agn_transcript_clique_to_gff3(predclique, v->fp_reports, " | ");
    printf_c(v->fp_reports, " |\n");
  }

  AgnComparison *pairstats = agn_clique_pair_get_stats(pair);
  locus_print_structure_report(v->fp_reports, &pairstats->cds_struc_stats,
                               "CDS", "CDS segments");
  locus_print_structure_report(v->fp_reports, &pairstats->exon_struc_stats,
                               "Exon", "exons");
  locus_print_structure_report(v->fp_reports, &pairstats->utr_struc_stats,
                               "UTR", "UTR segments");
  locus_print_nucleotide_report(v->fp_reports, pair);
}

static void locus_report_print_unmatched_cliques(AgnCompareTextReportVisitor *v,
                                                 AgnLocus *locus)
{
  GtUword i;

  GtArray *unique_refr_cliques = agn_locus_get_unique_refr_cliques(locus);
  if(unique_refr_cliques && gt_array_size(unique_refr_cliques) > 0)
  {
    printf_c(v->fp_reports, "     |\n");
    printf_c(v->fp_reports, "     |  reference transcripts (or transcript "
             "sets) without a prediction match\n");
  }
  for(i = 0; i < gt_array_size(unique_refr_cliques); i++)
  {
    AgnTranscriptClique **clique = gt_array_get(unique_refr_cliques, i);
    char *tid = agn_transcript_clique_id(*clique);
    printf_c(v->fp_reports, "     | [%s]\n", tid);
    gt_free(tid);
  }

  GtArray *unique_pred_cliques = agn_locus_get_unique_pred_cliques(locus);
  if(unique_pred_cliques && gt_array_size(unique_pred_cliques) > 0)
  {
    printf_c(v->fp_reports, "     |\n");
    printf_c(v->fp_reports, "     |  novel prediction transcripts (or "
             "transcript sets)\n");
  }
  for(i = 0; i < gt_array_size(unique_pred_cliques); i++)
  {
    AgnTranscriptClique **clique = gt_array_get(unique_pred_cliques, i);
    char *tid = agn_transcript_clique_id(*clique);
    printf_c(v->fp_reports, "     | [%s]\n", tid);
    gt_free(tid);
  }
}

static void print_locus_report(AgnCompareTextReportVisitor *v, AgnLocus *locus)
{
  GtRange range = gt_genome_node_get_range(locus);
  GtStr *seqid = gt_genome_node_get_seqid(locus);
  printf_c(v->fp_reports,
          "|-------------------------------------------------\n"
          "|---- Locus: %s_%lu-%lu\n"
          "|-------------------------------------------------\n"
          "|\n",
          gt_str_get(seqid), range.start, range.end);

  locus_report_print_geneids(v, locus);
  printf_c(v->fp_reports,
          "|\n"
          "|----------\n");

  GtArray *pairs2report = agn_locus_pairs_to_report(locus);
  if(pairs2report == NULL || gt_array_size(pairs2report) == 0)
  {
    printf_c(v->fp_reports,
            "     |\n"
            "     | No comparisons were performed for this locus.\n"
            "     |\n");
    return;
  }

  GtUword i;
  for(i = 0; i < gt_array_size(pairs2report); i++)
  {
    AgnCliquePair *pair = *(AgnCliquePair **)gt_array_get(pairs2report, i);
    printf_c(v->fp_reports,
            "     |\n"
            "     |--------------------------\n"
            "     |---- Begin comparison ----\n"
            "     |--------------------------\n"
            "     |\n");
    locus_report_print_pair(v, pair);
    printf_c(v->fp_reports,
            "     |\n"
            "     |--------------------------\n"
            "     |----- End comparison -----\n"
            "     |--------------------------\n"
            "     |\n");
  }

  locus_report_print_unmatched_cliques(v, locus);
  printf_c(v->fp_reports, "\n");
}

static void printf_c(FILE *outstream, const char *format, ...)
{
  va_list ap;
  if(outstream == NULL)
    return;

  va_start(ap, format);
  vfprintf(outstream, format, ap);
  va_end(ap);
}
