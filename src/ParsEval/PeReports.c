#include <math.h>
#include <string.h>
#include "AgnGtExtensions.h"
#include "PeReports.h"
#include "AgnUtils.h"

void pe_gene_locus_get_filename(AgnPairwiseCompareLocus *locus, char *buffer, const char *dirpath)
{
  const char *seqid = agn_pairwise_compare_locus_get_seqid(locus);
  sprintf( buffer, "%s/%s/%lu-%lu.html", dirpath, seqid,
           agn_pairwise_compare_locus_get_start(locus), agn_pairwise_compare_locus_get_end(locus) );
}

unsigned long pe_gene_locus_get_graphic_width(AgnPairwiseCompareLocus *locus)
{
  double scaling_factor = 0.05;
  unsigned long graphic_width = agn_pairwise_compare_locus_get_length(locus) * scaling_factor;
  if(graphic_width < AGN_PAIRWISE_COMPARE_LOCUS_GRAPHIC_MIN_WIDTH)
    graphic_width = AGN_PAIRWISE_COMPARE_LOCUS_GRAPHIC_MIN_WIDTH;
  return graphic_width;
}

void pe_gene_locus_get_png_filename(AgnPairwiseCompareLocus *locus, char *buffer, const char *dirpath)
{
  const char *seqid = agn_pairwise_compare_locus_get_seqid(locus);
  sprintf( buffer, "%s/%s/%s_%lu-%lu.png", dirpath, seqid, seqid,
           agn_pairwise_compare_locus_get_start(locus), agn_pairwise_compare_locus_get_end(locus) );
}

void pe_gene_locus_print_results(AgnPairwiseCompareLocus *locus, FILE *outstream, PeOptions *options)
{
  if(strcmp(options->outfmt, "csv") == 0)
  {
    pe_gene_locus_print_results_csv(locus, outstream, options);
    return;
  }
  else if(strcmp(options->outfmt, "html") == 0)
  {
    pe_gene_locus_print_results_html(locus, options);
    return;
  }

  fprintf(outstream, "|-------------------------------------------------\n");
  fprintf( outstream, "|---- Locus: sequence '%s' from %lu to %lu\n",
           agn_pairwise_compare_locus_get_seqid(locus), agn_pairwise_compare_locus_get_start(locus),
           agn_pairwise_compare_locus_get_end(locus) );
  fprintf(outstream, "|-------------------------------------------------\n");
  fprintf(outstream, "|\n");

  fprintf(outstream, "|  reference genes:\n");
  GtArray *refr_genes = agn_pairwise_compare_locus_get_refr_genes(locus);
  if(refr_genes == NULL || gt_array_size(refr_genes) == 0)
    fprintf(outstream, "|    None!\n");
  else
  {
    unsigned long i;
    for(i = 0; i < gt_array_size(refr_genes); i++)
    {
      GtFeatureNode *gene = *(GtFeatureNode **)gt_array_get(refr_genes, i);
      fprintf(outstream, "|    %s\n", gt_feature_node_get_attribute(gene, "ID"));
    }
  }
  fprintf(outstream, "|\n");

  fprintf(outstream, "|  prediction genes:\n");
  GtArray *pred_genes = agn_pairwise_compare_locus_get_pred_genes(locus);
  if(pred_genes == NULL || gt_array_size(pred_genes) == 0)
    fprintf(outstream, "|    None!\n");
  else
  {
    unsigned long i;
    for(i = 0; i < gt_array_size(pred_genes); i++)
    {
      GtFeatureNode *gene = *(GtFeatureNode **)gt_array_get(pred_genes, i);
      fprintf(outstream, "|    %s\n", gt_feature_node_get_attribute(gene, "ID"));
    }
  }
  fprintf(outstream, "|\n");

  fprintf(outstream, "|  locus splice complexity:\n");
  fprintf(outstream, "|    reference:   %.3lf\n", agn_pairwise_compare_locus_get_refr_splice_complexity(locus));
  fprintf(outstream, "|    prediction:  %.3lf\n", agn_pairwise_compare_locus_get_pred_splice_complexity(locus));
  fprintf(outstream, "|\n");

  fprintf(outstream, "|\n");
  fprintf(outstream, "|----------\n");

  GtArray *locuspairs = agn_pairwise_compare_locus_get_clique_pairs(locus, options->trans_per_locus);
  if(locuspairs == NULL)
  {
    fprintf(outstream, "     |\n");
    fprintf(outstream, "     |  No comparisons were performed for this locus\n");
    fprintf(outstream, "     |\n");
  }
  else if(options->complimit != 0 && gt_array_size(locuspairs) > options->complimit)
  {
    fprintf( outstream, "     |\n" );
    fprintf( outstream, "     |  No comparisons were performed for this locus. The number "
             "of transcript clique pairs (%lu) exceeds the limit of %d.\n",
             gt_array_size(locuspairs), options->complimit );
    fprintf( outstream, "     |\n" );
  }
  else
  {
    unsigned long k;
    GtArray *reported_pairs = agn_pairwise_compare_locus_find_best_pairs(locus);
    unsigned long pairs_to_report = gt_array_size(reported_pairs);
    gt_assert(pairs_to_report > 0 && reported_pairs != NULL);
    for(k = 0; k < pairs_to_report; k++)
    {
      AgnCliquePair *pair = *(AgnCliquePair **)gt_array_get(reported_pairs, k);
      gt_assert(agn_clique_pair_needs_comparison(pair));

      if(outstream != NULL)
      {
        fprintf(outstream, "     |\n");
        fprintf(outstream, "     |--------------------------\n");
        fprintf(outstream, "     |---- Begin Comparison ----\n");
        fprintf(outstream, "     |--------------------------\n");
        fprintf(outstream, "     |\n");
        fprintf(outstream, "     |  reference transcripts:\n");

        GtFeatureNode *transcript;
        AgnTranscriptClique *refr_clique = agn_clique_pair_get_refr_clique(pair);
        AgnTranscriptClique *pred_clique = agn_clique_pair_get_pred_clique(pair);

        while((transcript = agn_transcript_clique_next(refr_clique)))
        {
          fprintf( outstream, "     |    %s\n",
                   gt_feature_node_get_attribute(transcript, "ID"));
        }
        fprintf(outstream, "     |  prediction transcripts:\n");
        while((transcript = agn_transcript_clique_next(pred_clique)))
        {
          fprintf( outstream, "     |    %s\n",
                   gt_feature_node_get_attribute(transcript, "ID"));
        }
        fprintf(outstream, "     |\n");

        if(options->gff3)
        {
          fprintf(outstream, "     |  reference GFF3:\n");
          while((transcript = agn_transcript_clique_next(refr_clique)))
            agn_gt_feature_node_to_gff3(transcript, outstream, true, "     |    ", NULL);

          fprintf(outstream, "     |  prediction GFF3:\n");
          while((transcript = agn_transcript_clique_next(pred_clique)))
            agn_gt_feature_node_to_gff3(transcript, outstream, true, "     |    ", NULL);

          fprintf(outstream, "     |\n");
        }

        if(options->vectors)
        {
          fprintf(outstream, "     |  model vectors:\n");
          fprintf(outstream, "     |    refr: %s\n", agn_clique_pair_get_refr_vector(pair));
          fprintf(outstream, "     |    pred: %s\n", agn_clique_pair_get_pred_vector(pair));
          fprintf(outstream, "     |\n");
        }

        AgnComparisonStats *pairstats = agn_clique_pair_get_stats(pair);

        // CDS structure stats
        fprintf(outstream, "     |  CDS structure comparison\n");
        if(pairstats->cds_struc_stats.missing == 0 && pairstats->cds_struc_stats.wrong == 0)
        {
          fprintf( outstream, "     |    %lu reference CDS segments\n",
                   pairstats->cds_struc_stats.correct );
          fprintf( outstream, "     |    %lu prediction CDS segments\n",
                   pairstats->cds_struc_stats.correct );
          fprintf( outstream, "     |    CDS structures match perfectly!\n" );
        }
        else
        {
          fprintf( outstream, "     |    %lu reference CDS segments\n",
                   pairstats->cds_struc_stats.correct + pairstats->cds_struc_stats.missing );
          fprintf( outstream, "     |      %lu match prediction\n",
                   pairstats->cds_struc_stats.correct );
          fprintf( outstream, "     |      %lu don't match prediction\n",
                   pairstats->cds_struc_stats.missing );
          fprintf( outstream, "     |    %lu prediction CDS segments\n",
                   pairstats->cds_struc_stats.correct + pairstats->cds_struc_stats.wrong );
          fprintf( outstream, "     |      %lu match reference\n",
                   pairstats->cds_struc_stats.correct );
          fprintf( outstream, "     |      %lu don't match reference\n",
                   pairstats->cds_struc_stats.wrong );
          fprintf( outstream, "     |    %-30s %-10s\n", "Sensitivity:",
                   pairstats->cds_struc_stats.sns );
          fprintf( outstream, "     |    %-30s %-10s\n", "Specificity:",
                   pairstats->cds_struc_stats.sps );
          fprintf( outstream, "     |    %-30s %-10s\n", "F1 Score:",
                   pairstats->cds_struc_stats.f1s );
          fprintf( outstream, "     |    %-30s %-10s\n", "Annotation edit distance:",
                   pairstats->cds_struc_stats.eds );
        }
        fprintf(outstream, "     |\n");

        // Exon structure stats
        fprintf(outstream, "     |  Exon structure comparison\n");
        if(pairstats->exon_struc_stats.missing == 0 && pairstats->exon_struc_stats.wrong == 0)
        {
          fprintf( outstream, "     |    %lu reference exons\n",
                   pairstats->exon_struc_stats.correct );
          fprintf( outstream, "     |    %lu prediction exons\n",
                   pairstats->exon_struc_stats.correct );
          fprintf( outstream, "     |    Exon structures match perfectly!\n" );
        }
        else
        {
          fprintf( outstream, "     |    %lu reference exons\n",
                   pairstats->exon_struc_stats.correct + pairstats->exon_struc_stats.missing );
          fprintf( outstream, "     |      %lu match prediction\n",
                   pairstats->exon_struc_stats.correct );
          fprintf( outstream, "     |      %lu don't match prediction\n",
                   pairstats->exon_struc_stats.missing );
          fprintf( outstream, "     |    %lu prediction exons\n",
                   pairstats->exon_struc_stats.correct + pairstats->exon_struc_stats.wrong );
          fprintf( outstream, "     |      %lu match reference\n",
                   pairstats->exon_struc_stats.correct );
          fprintf( outstream, "     |      %lu don't match reference\n",
                   pairstats->exon_struc_stats.wrong );
          fprintf( outstream, "     |    %-30s %-10s\n", "Sensitivity:",
                   pairstats->exon_struc_stats.sns );
          fprintf( outstream, "     |    %-30s %-10s\n", "Specificity:",
                   pairstats->exon_struc_stats.sps );
          fprintf( outstream, "     |    %-30s %-10s\n", "F1 Score:",
                   pairstats->exon_struc_stats.f1s );
          fprintf( outstream, "     |    %-30s %-10s\n", "Annotation edit distance:",
                   pairstats->exon_struc_stats.eds );
        }
        fprintf(outstream, "     |\n");

        // UTR structure stats
        fprintf(outstream, "     |  UTR structure comparison\n");
        if(!agn_clique_pair_has_utrs(pair))
        {
          fprintf( outstream, "     |    No UTRs annotated for this locus.\n" );
        }
        else
        {
          if( agn_clique_pair_has_utrs(pair) &&
              pairstats->utr_struc_stats.missing == 0 &&
              pairstats->utr_struc_stats.wrong == 0 )
          {
            fprintf( outstream, "     |    %lu reference UTR segments\n",
                     pairstats->utr_struc_stats.correct );
            fprintf( outstream, "     |    %lu prediction UTR segments\n",
                     pairstats->utr_struc_stats.correct );
            fprintf( outstream, "     |    UTR structures match perfectly!\n" );
          }
          else
          {
            fprintf( outstream, "     |    %lu reference UTR segments\n",
                     pairstats->utr_struc_stats.correct + pairstats->utr_struc_stats.missing );
            fprintf( outstream, "     |      %lu match prediction\n",
                     pairstats->utr_struc_stats.correct );
            fprintf( outstream, "     |      %lu don't match prediction\n",
                     pairstats->utr_struc_stats.missing );
            fprintf( outstream, "     |    %lu prediction UTR segments\n",
                     pairstats->utr_struc_stats.correct + pairstats->utr_struc_stats.wrong );
            fprintf( outstream, "     |      %lu match reference\n",
                     pairstats->utr_struc_stats.correct );
            fprintf( outstream, "     |      %lu don't match reference\n",
                     pairstats->utr_struc_stats.wrong );
            fprintf( outstream, "     |    %-30s %-10s\n", "Sensitivity:",
                     pairstats->utr_struc_stats.sns );
            fprintf( outstream, "     |    %-30s %-10s\n", "Specificity:",
                     pairstats->utr_struc_stats.sps );
            fprintf( outstream, "     |    %-30s %-10s\n", "F1 Score:",
                     pairstats->utr_struc_stats.f1s );
            fprintf( outstream, "     |    %-30s %-10s\n", "Annotation edit distance:",
                     pairstats->utr_struc_stats.eds );
          }
        }
        fprintf(outstream, "     |\n");

        // Nucleotide-level stats
        if(fabs(pairstats->overall_identity - 1.0) < pairstats->tolerance)
            fprintf(outstream, "     |  Gene structures match perfectly!\n");
        else
        {
          fprintf( outstream, "     |  %-30s   %-10s   %-10s   %-10s\n",
                   "Nucleotide-level comparison", "CDS", "UTRs", "Overall" );
          fprintf( outstream, "     |    %-30s %-10s   %-10s   %.3lf\n",
                   "Matching coefficient:", pairstats->cds_nuc_stats.mcs,
                   pairstats->utr_nuc_stats.mcs, pairstats->overall_identity);
          fprintf( outstream, "     |    %-30s %-10s   %-10s   %-10s\n",
                   "Correlation coefficient:", pairstats->cds_nuc_stats.ccs,
                   pairstats->utr_nuc_stats.ccs, "--");
          fprintf( outstream, "     |    %-30s %-10s   %-10s   %-10s\n", "Sensitivity:",
                   pairstats->cds_nuc_stats.sns, pairstats->utr_nuc_stats.sns, "--");
          fprintf( outstream, "     |    %-30s %-10s   %-10s   %-10s\n", "Specificity:",
                   pairstats->cds_nuc_stats.sps, pairstats->utr_nuc_stats.sps, "--");
          fprintf( outstream, "     |    %-30s %-10s   %-10s   %-10s\n", "F1 Score:",
                   pairstats->cds_nuc_stats.f1s, pairstats->utr_nuc_stats.f1s, "--");
          fprintf( outstream, "     |    %-30s %-10s   %-10s   %-10s\n", "Annotation edit distance:",
                   pairstats->cds_nuc_stats.eds, pairstats->utr_nuc_stats.eds, "--");
        }

        fprintf(outstream, "     |\n");
        fprintf(outstream, "     |--------------------------\n");
        fprintf(outstream, "     |----- End Comparison -----\n");
        fprintf(outstream, "     |--------------------------\n");
      }
    }

    if(outstream != NULL)
    {
      GtArray *unique_refr_cliques = agn_pairwise_compare_locus_get_unique_refr_cliques(locus);
      if(gt_array_size(unique_refr_cliques) > 0)
      {
        fprintf(outstream, "     |\n");
        fprintf(outstream, "     |  reference transcripts (or transcript sets) without a "
                           "prediction match\n");
      }
      for(k = 0; k < gt_array_size(unique_refr_cliques); k++)
      {
        AgnTranscriptClique *clique = *(AgnTranscriptClique **)gt_array_get(unique_refr_cliques, k);
        fprintf(outstream, "     |    ");
        agn_transcript_clique_print_ids(clique, outstream);
        fprintf(outstream, "\n");
      }

      GtArray *unique_pred_cliques = agn_pairwise_compare_locus_get_unique_pred_cliques(locus);
      if(gt_array_size(unique_pred_cliques) > 0)
      {
        fprintf(outstream, "     |\n");
        fprintf(outstream, "     |  novel prediction transcripts (or transcript sets)\n");
      }
      for(k = 0; k < gt_array_size(unique_pred_cliques); k++)
      {
        AgnTranscriptClique *clique = *(AgnTranscriptClique **)gt_array_get(unique_pred_cliques, k);
        fprintf(outstream, "     |    ");
        agn_transcript_clique_print_ids(clique, outstream);
        fprintf(outstream, "\n");
      }
    }
  }
  if(outstream != NULL)
    fputs("\n", outstream);
}

void pe_gene_locus_print_results_csv(AgnPairwiseCompareLocus *locus, FILE *outstream, PeOptions *options)
{
  unsigned long i;
  GtArray *reported_pairs = agn_pairwise_compare_locus_find_best_pairs(locus);
  unsigned long pairs_to_report = gt_array_size(reported_pairs);

  for(i = 0; i < pairs_to_report; i++)
  {
    AgnCliquePair *pair = *(AgnCliquePair **)gt_array_get(reported_pairs, i);
    GtArray *clique_pairs = agn_pairwise_compare_locus_get_clique_pairs(locus, options->trans_per_locus);

    if( !(options->complimit != 0 && gt_array_size(clique_pairs) > options->complimit) &&
        agn_clique_pair_needs_comparison(pair) )
    {
      unsigned long j;
      GtArray *refr_ids = agn_pairwise_compare_locus_get_refr_transcript_ids(locus);
      GtArray *pred_ids = agn_pairwise_compare_locus_get_pred_transcript_ids(locus);

      fprintf(outstream, "%s,%lu,%lu,", agn_pairwise_compare_locus_get_seqid(locus), agn_pairwise_compare_locus_get_start(locus), agn_pairwise_compare_locus_get_end(locus));
      for(j = 0; j < gt_array_size(refr_ids); j++)
      {
        char *id = *(char **)gt_array_get(refr_ids, j);
        if(j > 0)
          fputc('|', outstream);
        fputs(id, outstream);
      }
      fputs(",", outstream);
      for(j = 0; j < gt_array_size(pred_ids); j++)
      {
        char *id = *(char **)gt_array_get(pred_ids, j);
        if(j > 0)
          fputc('|', outstream);
        fputs(id, outstream);
      }
      fputs(",", outstream);

      AgnComparisonStats *pairstats = agn_clique_pair_get_stats(pair);

      // Print CDS structure comparison
      fprintf( outstream,
               "%lu,%lu,%lu,%lu,%lu,%s,%s,%s,%s,",
               pairstats->cds_struc_stats.correct + pairstats->cds_struc_stats.missing,
               pairstats->cds_struc_stats.correct + pairstats->cds_struc_stats.wrong,
               pairstats->cds_struc_stats.correct,
               pairstats->cds_struc_stats.missing,
               pairstats->cds_struc_stats.wrong,
               pairstats->cds_struc_stats.sns,
               pairstats->cds_struc_stats.sps,
               pairstats->cds_struc_stats.f1s,
               pairstats->cds_struc_stats.eds );

      // Print exon structure comparison
      fprintf( outstream,
               "%lu,%lu,%lu,%lu,%lu,%s,%s,%s,%s,",
               pairstats->exon_struc_stats.correct + pairstats->exon_struc_stats.missing,
               pairstats->exon_struc_stats.correct + pairstats->exon_struc_stats.wrong,
               pairstats->exon_struc_stats.correct,
               pairstats->exon_struc_stats.missing,
               pairstats->exon_struc_stats.wrong,
               pairstats->exon_struc_stats.sns,
               pairstats->exon_struc_stats.sps,
               pairstats->exon_struc_stats.f1s,
               pairstats->exon_struc_stats.eds );

      // Print UTR structure comparison
      fprintf( outstream,
               "%lu,%lu,%lu,%lu,%lu,%s,%s,%s,%s,",
               pairstats->utr_struc_stats.correct + pairstats->utr_struc_stats.missing,
               pairstats->utr_struc_stats.correct + pairstats->utr_struc_stats.wrong,
               pairstats->utr_struc_stats.correct,
               pairstats->utr_struc_stats.missing,
               pairstats->utr_struc_stats.wrong,
               pairstats->utr_struc_stats.sns,
               pairstats->utr_struc_stats.sps,
               pairstats->utr_struc_stats.f1s,
               pairstats->utr_struc_stats.eds );

      // Print nucleotide-level comparison stats
      fprintf( outstream,
               "%.3lf,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
               pairstats->overall_identity,
               pairstats->cds_nuc_stats.mcs,
               pairstats->cds_nuc_stats.ccs,
               pairstats->cds_nuc_stats.sns,
               pairstats->cds_nuc_stats.sps,
               pairstats->cds_nuc_stats.f1s,
               pairstats->cds_nuc_stats.eds,
               pairstats->utr_nuc_stats.mcs,
               pairstats->utr_nuc_stats.ccs,
               pairstats->utr_nuc_stats.sns,
               pairstats->utr_nuc_stats.sps,
               pairstats->utr_nuc_stats.f1s,
               pairstats->utr_nuc_stats.eds );

      fputc('\n', outstream);
    }
  }
}

void pe_gene_locus_print_results_html(AgnPairwiseCompareLocus *locus, PeOptions *options)
{
  char filename[512];
  pe_gene_locus_get_filename(locus, filename, options->outfilename);
  FILE *outstream = agn_fopen(filename, "w");

  fprintf( outstream,
           "<!doctype html>\n"
           "<html lang=\"en\">\n"
           "  <head>\n"
           "    <meta charset=\"utf-8\" />\n"
           "    <title>ParsEval: Locus at %s[%lu, %lu]</title>\n"
           "    <link rel=\"stylesheet\" type=\"text/css\" href=\"../parseval.css\" />\n",
           agn_pairwise_compare_locus_get_seqid(locus), agn_pairwise_compare_locus_get_start(locus),
           agn_pairwise_compare_locus_get_end(locus) );

  GtArray *clique_pairs = agn_pairwise_compare_locus_get_clique_pairs(locus, options->trans_per_locus);
  if( clique_pairs != NULL &&
      (options->complimit == 0 || gt_array_size(clique_pairs) <= options->complimit) )
  {
    fputs( "    <script type=\"text/javascript\""
           " src=\"../mootools-core-1.3.2-full-nocompat-yc.js\"></script>\n"
           "    <script type=\"text/javascript\" src=\"../mootools-more-1.3.2.1.js\"></script>\n"
           "    <script type=\"text/javascript\">\n"
           "window.addEvent('domready', function() {\n"
           "  var status =\n"
           "  {\n"
           "    'true': \"(hide details)\",\n"
           "    'false': \"(show details)\",\n"
           "  }\n",
           outstream);
    unsigned long i;
    GtArray *reported_pairs = agn_pairwise_compare_locus_find_best_pairs(locus);
    for(i = 0; i < gt_array_size(reported_pairs); i++)
    {
      fprintf( outstream,
               "  var compareWrapper%lu = new Fx.Slide('compare_wrapper_%lu');\n"
               "  compareWrapper%lu.hide();\n"
               "  $('toggle_compare_%lu').addEvent('click', function(event){\n"
               "    event.stop();\n"
               "    compareWrapper%lu.toggle();\n"
               "  });\n"
               "  compareWrapper%lu.addEvent('complete', function() {\n"
               "    $('toggle_compare_%lu').set('text', status[compareWrapper%lu.open]);\n"
               "  });\n",
               i, i, i, i, i, i, i, i);
    }
    fputs( "});\n"
           "    </script>\n",
           outstream);
  }

  fprintf( outstream,
           "  </head>\n"
           "  <body>\n"
           "    <div id=\"content\">\n"
           "      <h1>Locus at %s[%lu, %lu]</h1>\n"
           "      <p><a href=\"index.html\">⇐ Back to %s loci</a></p>\n\n",
           agn_pairwise_compare_locus_get_seqid(locus), agn_pairwise_compare_locus_get_start(locus),
           agn_pairwise_compare_locus_get_end(locus), agn_pairwise_compare_locus_get_seqid(locus) );

  fputs( "      <h2>Gene annotations</h2>\n"
         "      <table>\n"
         "        <tr><th>Reference</th><th>Prediction</th></tr>\n",
         outstream );
  GtArray *refr_genes = agn_pairwise_compare_locus_get_refr_gene_ids(locus);
  GtArray *pred_genes = agn_pairwise_compare_locus_get_pred_gene_ids(locus);
  unsigned long i;
  for(i = 0; i < gt_array_size(refr_genes) || i < gt_array_size(pred_genes); i++)
  {
    fputs("        <tr>", outstream);
    if(i < gt_array_size(refr_genes))
    {
      const char *id = *(char **)gt_array_get(refr_genes, i);
      fprintf(outstream, "<td>%s</td>", id);
    }
    else
    {
      if(i == 0)
        fputs("<td>None</td>", outstream);
      else
        fputs("<td>&nbsp;</td>", outstream);
    }

    if(i < gt_array_size(pred_genes))
    {
      const char *id = *(char **)gt_array_get(pred_genes, i);
      fprintf(outstream, "<td>%s</td>", id);
    }
    else
    {
      if(i == 0)
        fputs("<td>None</td>", outstream);
      else
        fputs("<td>&nbsp;</td>", outstream);
    }
    fputs("</tr>\n", outstream);
  }
  fputs("      </table>\n\n", outstream);

  fputs( "      <h2>Transcript annotations</h2>\n"
         "      <table>\n"
         "        <tr><th>Reference</th><th>Prediction</th></tr>\n",
         outstream );
  GtArray *refr_trns = agn_pairwise_compare_locus_get_refr_transcript_ids(locus);
  GtArray *pred_trns = agn_pairwise_compare_locus_get_pred_transcript_ids(locus);
  for(i = 0; i < gt_array_size(refr_trns) || i < gt_array_size(pred_trns); i++)
  {
    fputs("      <tr>", outstream);
    if(i < gt_array_size(refr_trns))
    {
      const char *id = *(char **)gt_array_get(refr_trns, i);
      fprintf(outstream, "<td>%s</td>", id);
    }
    else
    {
      if(i == 0)
        fputs("<td>None</td>", outstream);
      else
        fputs("<td>&nbsp;</td>", outstream);
    }

    if(i < gt_array_size(pred_trns))
    {
      const char *id = *(char **)gt_array_get(pred_trns, i);
      fprintf(outstream, "<td>%s</td>", id);
    }
    else
    {
      if(i == 0)
        fputs("<td>None</td>", outstream);
      else
        fputs("<td>&nbsp;</td>", outstream);
    }
    fputs("</tr>\n", outstream);
  }
  fputs("      </table>\n\n", outstream);

  fputs("      <h2>Locus splice complexity</h2>\n", outstream);
  fputs("      <table>\n", outstream);
  fputs("        <tr><th>Reference</th><th>Prediction</th></tr>\n", outstream);
  fprintf( outstream, "        <tr><td>%.3lf</td><td>%.3lf</td></tr>\n",
           agn_pairwise_compare_locus_get_refr_splice_complexity(locus), agn_pairwise_compare_locus_get_pred_splice_complexity(locus) );
  fputs("      </table>\n", outstream);

  if(options->locus_graphics)
  {
    fputs("      <div class=\"graphic\">\n      ", outstream);
    if(pe_gene_locus_get_graphic_width(locus) > AGN_PAIRWISE_COMPARE_LOCUS_GRAPHIC_MIN_WIDTH)
    {
      fprintf( outstream, "<a href=\"%s_%lu-%lu.png\">",
               agn_pairwise_compare_locus_get_seqid(locus), agn_pairwise_compare_locus_get_start(locus),
               agn_pairwise_compare_locus_get_end(locus) );
    }
    fprintf( outstream, "<img src=\"%s_%lu-%lu.png\" />\n",
             agn_pairwise_compare_locus_get_seqid(locus), agn_pairwise_compare_locus_get_start(locus),
             agn_pairwise_compare_locus_get_end(locus) );
    if(pe_gene_locus_get_graphic_width(locus) > AGN_PAIRWISE_COMPARE_LOCUS_GRAPHIC_MIN_WIDTH)
    {
      fputs("</a>", outstream);
    }
    fputs("      </div>\n\n", outstream);
  }

  if(agn_pairwise_compare_locus_get_clique_pairs(locus, options->trans_per_locus) == NULL)
  {
    // ???
  }
  else if(options->complimit != 0 && gt_array_size(clique_pairs) > options->complimit)
  {
    fprintf( outstream, "      <p>No comparisons were performed for this locus. The number "
             "of transcript clique pairs (%lu) exceeds the limit of %d.</p>\n\n",
             gt_array_size(clique_pairs), options->complimit );
  }
  else
  {
    fputs("      <h2 class=\"bottomspace\">Comparisons</h2>\n", outstream);

    unsigned long k;
    GtArray *reported_pairs = agn_pairwise_compare_locus_find_best_pairs(locus);
    unsigned long pairs_to_report = gt_array_size(reported_pairs);
    gt_assert(pairs_to_report > 0 && reported_pairs != NULL);
    for(k = 0; k < pairs_to_report; k++)
    {
      AgnCliquePair *pair = *(AgnCliquePair **)gt_array_get(reported_pairs, k);
      gt_assert(agn_clique_pair_needs_comparison(pair));
      AgnTranscriptClique *refrclique = agn_clique_pair_get_refr_clique(pair);
      AgnTranscriptClique *predclique = agn_clique_pair_get_pred_clique(pair);

      // What to put in compare-header? Some cliques may have alot of IDs.
      // Do I just use arbitrary numbers?
      if(agn_clique_pair_is_simple(pair))
      {
        char refr_id[20 + 1], pred_id[20 + 1];
        GtFeatureNode *refr_trans = agn_transcript_clique_next(refrclique);
        GtFeatureNode *pred_trans = agn_transcript_clique_next(predclique);
        agn_transcript_clique_reset(refrclique);
        agn_transcript_clique_reset(predclique);

        agn_gt_feature_node_get_trimmed_id(refr_trans, refr_id, 20);
        agn_gt_feature_node_get_trimmed_id(pred_trans, pred_id, 20);

        fprintf( outstream,
                 "      <h3 class=\"compare-header\">%s vs %s "
                 "<a id=\"toggle_compare_%lu\" href=\"#\">(show details)</a></h3>\n",
                 refr_id, pred_id, k );
      }
      else
      {
        fprintf( outstream,
                 "      <h3 class=\"compare-header\">Complex comparison "
                 "<a id=\"toggle_compare_%lu\" href=\"#\">(show details)</a></h3>\n",
                 k );
      }

      fprintf(outstream, "      <div id=\"compare_wrapper_%lu\" class=\"compare-wrapper\">\n", k);
      if(options->gff3)
      {
        GtFeatureNode *transcript;

        fputs( "        <h3>Reference GFF3</h3>\n"
               "        <pre class=\"gff3 refr\">\n",
               outstream);
        while((transcript = agn_transcript_clique_next(refrclique)))
          agn_gt_feature_node_to_gff3(transcript, outstream, true, NULL, NULL);
        fputs("</pre>\n", outstream);

        fputs( "        <h3>Prediction GFF3</h3>\n"
               "        <pre class=\"gff3 pred\">\n",
               outstream);
        while((transcript = agn_transcript_clique_next(predclique)))
          agn_gt_feature_node_to_gff3(transcript, outstream, true, NULL, NULL);
        fputs("</pre>\n", outstream);
      }

      if(options->vectors)
      {
        fprintf( outstream,
                 "        <h3>Model vectors</h3>\n"
                 "        <pre class=\"vectors\">\n"
                 "<span class=\"refr_vector\">%s</span>\n"
                 "<span class=\"pred_vector\">%s</span></pre>\n\n",
                 agn_clique_pair_get_refr_vector(pair),
                 agn_clique_pair_get_pred_vector(pair) );
      }

      AgnComparisonStats *pairstats = agn_clique_pair_get_stats(pair);

      // CDS structure stats
      fputs( "        <h3>CDS structure comparison</h3>\n"
             "        <table class=\"table_normal table_extra_indent\">\n",
             outstream );
      if(pairstats->cds_struc_stats.missing == 0 && pairstats->cds_struc_stats.wrong == 0)
      {
        fprintf( outstream,
                 "          <tr><td>reference CDS segments</td><td>%lu</td></tr>\n"
                 "          <tr><td>prediction CDS segments</td><td>%lu</td></tr>\n"
                 "          <tr><th class=\"left-align\" colspan=\"2\">CDS structures match "
                 "perfectly!</th></tr>\n",
                 pairstats->cds_struc_stats.correct, pairstats->cds_struc_stats.correct );
      }
      else
      {
        fprintf( outstream,
                 "          <tr><td>reference CDS segments</td><td>%lu</td></tr>\n"
                 "          <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction"
                 "</td><td>%lu</td></tr>\n"
                 "          <tr class=\"cell_small\"><td class=\"cell_indent\">don't match"
                 " prediction</td><td>%lu</td></tr>\n"
                 "          <tr><td>prediction CDS segments</td><td>%lu</td></tr>\n"
                 "          <tr class=\"cell_small\"><td class=\"cell_indent\">match reference"
                 "</td><td>%lu</td></tr>\n"
                 "          <tr class=\"cell_small\"><td class=\"cell_indent\">don't match"
                 " reference</td><td>%lu</td></tr>\n"
                 "          <tr><td>sensitivity</td><td>%-10s</td></tr>\n"
                 "          <tr><td>specificity</td><td>%-10s</td></tr>\n"
                 "          <tr><td>F1 score</td><td>%-10s</td></tr>\n"
                 "          <tr><td>Annotation edit distance</td><td>%-10s</td></tr>\n",
                 pairstats->cds_struc_stats.correct + pairstats->cds_struc_stats.missing,
                 pairstats->cds_struc_stats.correct, pairstats->cds_struc_stats.missing,
                 pairstats->cds_struc_stats.correct + pairstats->cds_struc_stats.wrong,
                 pairstats->cds_struc_stats.correct, pairstats->cds_struc_stats.wrong,
                 pairstats->cds_struc_stats.sns, pairstats->cds_struc_stats.sps,
                 pairstats->cds_struc_stats.f1s, pairstats->cds_struc_stats.eds );
      }
      fputs("        </table>\n\n", outstream);

      // Exon structure stats
      fputs( "        <h3>Exon structure comparison</h3>\n"
             "        <table class=\"table_normal table_extra_indent\">\n",
             outstream );
      if(pairstats->exon_struc_stats.missing == 0 && pairstats->exon_struc_stats.wrong == 0)
      {
        fprintf( outstream,
                 "          <tr><td>reference exons</td><td>%lu</td></tr>\n"
                 "          <tr><td>prediction exons</td><td>%lu</td></tr>\n"
                 "          <tr><th class=\"left-align\" colspan=\"2\">Exon structures match "
                 "perfectly!</th></tr>\n",
                 pairstats->exon_struc_stats.correct, pairstats->exon_struc_stats.correct );
      }
      else
      {
        fprintf( outstream,
                 "          <tr><td>reference exons</td><td>%lu</td></tr>\n"
                 "          <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction"
                 "</td><td>%lu</td></tr>\n"
                 "          <tr class=\"cell_small\"><td class=\"cell_indent\">don't match"
                 " prediction</td><td>%lu</td></tr>\n"
                 "          <tr><td>prediction exons</td><td>%lu</td></tr>\n"
                 "          <tr class=\"cell_small\"><td class=\"cell_indent\">match reference</td>"
                 "<td>%lu</td></tr>\n"
                 "          <tr class=\"cell_small\"><td class=\"cell_indent\">don't match"
                 " reference</td><td>%lu</td></tr>\n"
                 "          <tr><td>sensitivity</td><td>%-10s</td></tr>\n"
                 "          <tr><td>specificity</td><td>%-10s</td></tr>\n"
                 "          <tr><td>F1 score</td><td>%-10s</td></tr>\n"
                 "          <tr><td>Annotation edit distance</td><td>%-10s</td></tr>\n",
                 pairstats->exon_struc_stats.correct + pairstats->exon_struc_stats.missing,
                 pairstats->exon_struc_stats.correct, pairstats->exon_struc_stats.missing,
                 pairstats->exon_struc_stats.correct + pairstats->exon_struc_stats.wrong,
                 pairstats->exon_struc_stats.correct, pairstats->exon_struc_stats.wrong,
                 pairstats->exon_struc_stats.sns, pairstats->exon_struc_stats.sps,
                 pairstats->exon_struc_stats.f1s, pairstats->exon_struc_stats.eds );
      }
      fputs("        </table>\n\n", outstream);

      // UTR structure stats
      fputs("        <h3>UTR structure comparison</h3>\n", outstream);
      if(!agn_clique_pair_has_utrs(pair))
      {
        fputs("        <p class=\"no_utrs\">No UTRs annotated for this locus</p>\n\n", outstream);
      }
      else
      {
        fputs( "        <table class=\"table_normal table_extra_indent\">\n",
               outstream );
        if( agn_clique_pair_has_utrs(pair) &&
            pairstats->utr_struc_stats.missing == 0 &&
            pairstats->utr_struc_stats.wrong == 0 )
        {
          fprintf( outstream,
                   "          <tr><td>reference UTR segments</td><td>%lu</td></tr>\n"
                   "          <tr><td>prediction UTR segments</td><td>%lu</td></tr>\n"
                   "          <tr><th class=\"left-align\" colspan=\"2\">UTR structures match "
                   "perfectly!</th></tr>\n",
                   pairstats->utr_struc_stats.correct, pairstats->utr_struc_stats.correct );
        }
        else
        {
          fprintf( outstream,
                   "          <tr><td>reference UTR segments</td><td>%lu</td></tr>\n"
                   "          <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction"
                   "</td><td>%lu</td></tr>\n"
                   "          <tr class=\"cell_small\"><td class=\"cell_indent\">don't match"
                   " prediction</td><td>%lu</td></tr>\n"
                   "          <tr><td>prediction UTR segments</td><td>%lu</td></tr>\n"
                   "          <tr class=\"cell_small\"><td class=\"cell_indent\">match reference</td>"
                   "<td>%lu</td></tr>\n"
                   "          <tr class=\"cell_small\"><td class=\"cell_indent\">don't match"
                   " reference</td><td>%lu</td></tr>\n"
                   "          <tr><td>sensitivity</td><td>%-10s</td></tr>\n"
                   "          <tr><td>specificity</td><td>%-10s</td></tr>\n"
                   "          <tr><td>F1 score</td><td>%-10s</td></tr>\n"
                   "          <tr><td>Annotation edit distance</td><td>%-10s</td></tr>\n",
                   pairstats->utr_struc_stats.correct + pairstats->utr_struc_stats.missing,
                   pairstats->utr_struc_stats.correct, pairstats->utr_struc_stats.missing,
                   pairstats->utr_struc_stats.correct + pairstats->utr_struc_stats.wrong,
                   pairstats->utr_struc_stats.correct, pairstats->utr_struc_stats.wrong,
                   pairstats->utr_struc_stats.sns, pairstats->utr_struc_stats.sps,
                   pairstats->utr_struc_stats.f1s, pairstats->utr_struc_stats.eds );
        }
        fputs("        </table>\n\n", outstream);
      }

      // Nucleotide-level stats
      if(fabs(pairstats->overall_identity - 1.0) < pairstats->tolerance)
      {
        fputs("        <h3>Gene structures match perfectly!</h3>\n", outstream);
      }
      else
      {
        fprintf( outstream,
                 "        <h3>Nucleotide-level comparison</h3>\n"
                 "        <table class=\"table_wide table_extra_indent\">\n"
                 "          <tr><td>&nbsp;</td><th>CDS</th><th>UTRs</th><th>Overall</th></tr>\n"
                 "          <tr><th class=\"left-align\">matching coefficient</th><td>%-10s</td>"
                 "<td>%-10s</td><td>%.3f</td></tr>\n"
                 "          <tr><th class=\"left-align\">correlation coefficient</th><td>%-10s</td>"
                 "<td>%-10s</td><td>--</td></tr>\n"
                 "          <tr><th class=\"left-align\">sensitivity</th><td>%-10s</td>"
                 "<td>%-10s</td><td>--</td></tr>\n"
                 "          <tr><th class=\"left-align\">specificity</th><td>%-10s</td>"
                 "<td>%-10s</td><td>--</td></tr>\n"
                 "          <tr><th class=\"left-align\">F1 Score</th><td>%-10s</td>"
                 "<td>%-10s</td><td>--</td></tr>\n"
                 "          <tr><th class=\"left-align\">Annotation edit distance</th><td>%-10s</td>"
                 "<td>%-10s</td><td>--</td></tr>\n"
                 "        </table>\n",
                 pairstats->cds_nuc_stats.mcs, pairstats->utr_nuc_stats.mcs,
                 pairstats->overall_identity, pairstats->cds_nuc_stats.ccs,
                 pairstats->utr_nuc_stats.ccs, pairstats->cds_nuc_stats.sns,
                 pairstats->utr_nuc_stats.sns, pairstats->cds_nuc_stats.sps,
                 pairstats->utr_nuc_stats.sps, pairstats->cds_nuc_stats.f1s,
                 pairstats->utr_nuc_stats.f1s, pairstats->cds_nuc_stats.eds,
                 pairstats->utr_nuc_stats.eds );
      }

      fputs("      </div>\n\n", outstream);
    }

    GtArray *unique_refr_cliques = agn_pairwise_compare_locus_get_unique_refr_cliques(locus);
    if(gt_array_size(unique_refr_cliques) > 0)
    {
      fputs( "      <h2>Unmatched reference transcripts</h2>\n"
             "      <ul>\n",
             outstream);
      for(k = 0; k < gt_array_size(unique_refr_cliques); k++)
      {
        AgnTranscriptClique *clique = *(AgnTranscriptClique **)gt_array_get(unique_refr_cliques, k);
        fputs("        <li>", outstream);
        agn_transcript_clique_print_ids(clique, outstream);
        fputs("</li>\n", outstream);
      }
      fputs("      </ul>\n\n", outstream);
    }

    GtArray *unique_pred_cliques = agn_pairwise_compare_locus_get_unique_pred_cliques(locus);
    if(gt_array_size(unique_pred_cliques) > 0)
    {
      fputs( "      <h2>Novel prediction transcripts</h2>\n"
             "      <ul>\n",
             outstream);
      for(k = 0; k < gt_array_size(unique_pred_cliques); k++)
      {
        AgnTranscriptClique *clique = *(AgnTranscriptClique **)gt_array_get(unique_pred_cliques, k);
        fputs("        <li>", outstream);
        agn_transcript_clique_print_ids(clique, outstream);
        fputs("</li>\n", outstream);
      }
      fputs("      </ul>\n\n", outstream);
    }

  }

  pe_print_html_footer(outstream);
  fputs( "    </div>\n"
         "  </body>\n"
         "</html>",
         outstream );

  fclose(outstream);
}

void pe_print_html_footer(FILE *outstream)
{
  fprintf( outstream,
           "      <p class=\"footer\">\n"
           "        Generated by ParsEval (AEGeAn version %s).<br />\n"
           "        Copyright © %s <a href=\"http://parseval.sourceforge.net/contact.html\">"
           "ParsEval authors</a>.<br />\n"
           "        See <a href=\"LICENSE\">LICENSE</a> for details."
           "      </p>\n", AEGEAN_VERSION, AEGEAN_COPY_DATE);
}

void pe_print_locus_to_seqfile( FILE *seqfile, unsigned long start, unsigned long end,
                                unsigned long length, unsigned long refr_transcripts,
                                unsigned long pred_transcripts,
                                AgnComparisonCounts *comparisons )
{
  char sstart[64], send[64], slength[64];
  agn_sprintf_comma(start, sstart);
  agn_sprintf_comma(end, send);
  agn_sprintf_comma(length, slength);
  fprintf( seqfile,
           "        <tr>\n"
           "          <td><a href=\"%lu-%lu.html\">(+)</a></td>\n"
           "          <td>%s</td>\n"
           "          <td>%s</td>\n"
           "          <td>%s</td>\n"
           "          <td>%lu / %lu</td>\n"
           "          <td>\n"
           "            <a class=\"pointer\" title=\"Perfect matches at this locus\">[P]</a> %u\n"
           "            <a class=\"pointer left20\" title=\"Perfect matches at this locus with mislabeled UTRs\">[M]</a> %u\n"
           "            <a class=\"pointer left20\" title=\"CDS matches at this locus\">[C]</a> %u\n"
           "            <a class=\"pointer left20\" title=\"Exon structure matches at this locus\">[E]</a> %u\n"
           "            <a class=\"pointer left20\" title=\"UTR matches at this locus\">[U]</a> %u\n"
           "            <a class=\"pointer left20\" title=\"Non-matches at this locus\">[N]</a> %u\n"
           "          </td>\n"
           "        </tr>\n",
          start, end, sstart, send, slength, refr_transcripts, pred_transcripts, comparisons->num_perfect,
          comparisons->num_mislabeled, comparisons->num_cds_match, comparisons->num_exon_match,
          comparisons->num_utr_match, comparisons->non_match );
}

void pe_print_seqfile_header(FILE *outstream, const char *seqid)
{
  fprintf( outstream,
           "<!doctype html>\n"
           "<html lang=\"en\">\n"
           "  <head>\n"
           "    <meta charset=\"utf-8\" />\n"
           "    <title>ParsEval: Loci for %s</title>\n"
           "    <link rel=\"stylesheet\" type=\"text/css\" href=\"../parseval.css\" />\n"
           "    <script type=\"text/javascript\" language=\"javascript\" src=\"../jquery.js\"></script>\n"
           "    <script type=\"text/javascript\" language=\"javascript\" src=\"../jquery.dataTables.js\"></script>\n"
           "    <script type=\"text/javascript\">\n"
           "      $(document).ready(function() {\n"
           "        $('#locus_table').dataTable( {\n"
           "          \"sScrollY\": \"400px\",\n"
           "          \"bPaginate\": false,\n"
           "          \"bScrollCollapse\": true,\n"
           "          \"bSort\": false,\n"
           "          \"bFilter\": false,\n"
           "          \"bInfo\": false\n"
           "        });\n"
           "      } );\n"
           "    </script>\n"
           "  </head>\n"
           "  <body>\n"
           "    <div id=\"content\">\n"
           "      <h1>Loci for %s</h1>\n"
           "      <p><a href=\"../index.html\">⇐ Back to summary</a></p>\n\n"
           "      <p class=\"indent\">\n"
           "        Below is a list of all loci identified for sequence <strong>%s</strong>.\n"
           "        Click on the <a>(+)</a> symbol for a report of the complete comparative analysis corresponding to each locus.\n"
           "      </p>\n\n"
           "      <table class=\"loci\" id=\"locus_table\">\n"
           "        <thead>\n"
           "          <tr>\n"
           "            <th>&nbsp;</th>\n"
           "            <th>Start</th>\n"
           "            <th>End</th>\n"
           "            <th>Length</th>\n"
           "            <th>#Trans</th>\n"
           "            <th>Comparisons</th>\n"
           "          </tr>\n"
           "        </thead>\n"
           "        <tbody>\n",
           seqid, seqid, seqid );
  fflush(outstream);
}

void pe_print_seqfile_footer(FILE *outstream)
{
  fputs("        </tbody>\n", outstream);
  fputs("      </table>\n\n", outstream);
  pe_print_html_footer(outstream);
  fputs("    </div>\n", outstream);
  fputs("  </body>\n", outstream);
  fputs("</html>\n", outstream);
}

void pe_print_summary( const char *start_time, int argc, char * const argv[], GtStrArray *seqids,
                       AgnSummaryData *summary_data, AgnSummaryData *seq_summary_data,
                       FILE *outstream, PeOptions *options )
{
  // Calculate nucleotide-level statistics
  agn_resolve_nucleotide_level_stats(&summary_data->stats.cds_nuc_stats);
  agn_resolve_nucleotide_level_stats(&summary_data->stats.utr_nuc_stats);
  summary_data->stats.overall_identity = (double)summary_data->stats.overall_matches /
                                    (double)summary_data->stats.overall_length;

  // Calculate structure-level statistics
  agn_resolve_structure_level_stats(&summary_data->stats.cds_struc_stats);
  agn_resolve_structure_level_stats(&summary_data->stats.exon_struc_stats);
  agn_resolve_structure_level_stats(&summary_data->stats.utr_struc_stats);

  if(options->html)
  {
    pe_print_summary_html( start_time, argc, argv, seqids, summary_data, seq_summary_data,
                           outstream, options );
    return;
  }

  if(strcmp(options->outfmt, "csv") == 0)
    return;

  fprintf( outstream,
           "============================================================\n");
  fprintf( outstream, "========== ParsEval Summary\n");
  fprintf( outstream,
           "============================================================\n");
  fprintf(outstream, "Started:                %s\n", start_time);
  if(strcmp(options->refrlabel, "") != 0)
    fprintf(outstream, "Reference annotations:  %s\n", options->refrlabel);
  else
    fprintf(outstream, "Reference annotations:  %s\n", options->refrfile);
  if(strcmp(options->predlabel, "") != 0)
    fprintf(outstream, "Prediction annotations: %s\n", options->predlabel);
  else
    fprintf(outstream, "Prediction annotations: %s\n", options->predfile);
  fprintf(outstream, "Executing command:      ");
  int x;
  for(x = 0; x < argc; x++)
  {
    fprintf(outstream, "%s ", argv[x]);
  }
  fprintf(outstream, "\n\n");


  fprintf(outstream, "  Sequences compared\n");
  unsigned long i;
  for(i = 0; i < gt_str_array_size(seqids); i++)
  {
    const char *seqid = gt_str_array_get(seqids, i);
    fprintf(options->outfile, "    %s\n", seqid);
  }

  fprintf( outstream, "\n  Gene loci................................%lu\n",
           summary_data->counts.num_loci );
  fprintf( outstream, "    shared.................................%lu\n",
           summary_data->counts.num_loci - summary_data->counts.unique_refr - summary_data->counts.unique_pred );
  fprintf( outstream, "    unique to reference....................%d\n",
           summary_data->counts.unique_refr );
  fprintf( outstream, "    unique to prediction...................%d\n\n",
           summary_data->counts.unique_pred );

  fprintf( outstream, "  Reference annotations\n" );
  fprintf( outstream, "    genes..................................%lu\n",
           summary_data->counts.refr_genes );
  fprintf( outstream, "      average per locus....................%.3f\n",
           (float)summary_data->counts.refr_genes / (float)summary_data->counts.num_loci );
  fprintf( outstream, "    transcripts............................%lu\n",
           summary_data->counts.refr_transcripts );
  fprintf( outstream, "      average per locus....................%.3f\n",
           (float)summary_data->counts.refr_transcripts / (float)summary_data->counts.num_loci );
  fprintf( outstream, "      average per gene.....................%.3f\n\n",
           (float)summary_data->counts.refr_transcripts / (float)summary_data->counts.refr_genes );

  fprintf( outstream, "  Prediction annotations\n" );
  fprintf( outstream, "    genes..................................%lu\n",
           summary_data->counts.pred_genes );
  fprintf( outstream, "      average per locus....................%.3f\n",
           (float)summary_data->counts.pred_genes / (float)summary_data->counts.num_loci );
  fprintf( outstream, "    transcripts............................%lu\n",
           summary_data->counts.pred_transcripts );
  fprintf( outstream, "      average per locus....................%.3f\n",
           (float)summary_data->counts.pred_transcripts / (float)summary_data->counts.num_loci );
  fprintf( outstream, "      average per gene.....................%.3f\n\n",
           (float)summary_data->counts.pred_transcripts / (float)summary_data->counts.pred_genes );

  fprintf( outstream, "  Total comparisons........................%d\n",
           summary_data->counts.num_comparisons );
  fprintf( outstream, "    perfect matches........................%d (%.1f%%)\n",
           summary_data->counts.num_perfect,
           ((float)summary_data->counts.num_perfect / (float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.num_perfect > 0)
  {
    fprintf( outstream, "      avg. length..........................%.2lf bp\n",
             (double)summary_data->results.perfect_matches.total_length /
             (double)summary_data->results.perfect_matches.transcript_count );
    fprintf( outstream, "      avg. # refr exons....................%.2lf\n",
             (double)summary_data->results.perfect_matches.refr_exon_count /
             (double)summary_data->results.perfect_matches.transcript_count );
    fprintf( outstream, "      avg. # pred exons....................%.2lf\n",
             (double)summary_data->results.perfect_matches.pred_exon_count /
             (double)summary_data->results.perfect_matches.transcript_count );
    fprintf( outstream, "      avg. refr CDS length.................%.2lf aa\n",
             (double)summary_data->results.perfect_matches.refr_cds_length /
             (double)summary_data->results.perfect_matches.transcript_count );
    fprintf( outstream, "      avg. pred CDS length.................%.2lf aa\n",
             (double)summary_data->results.perfect_matches.pred_cds_length /
             (double)summary_data->results.perfect_matches.transcript_count );
  }
  fprintf( outstream, "    perfect matches with mislabeled UTRs...%d (%.1f%%)\n",
           summary_data->counts.num_mislabeled,
           ((float)summary_data->counts.num_mislabeled/(float)summary_data->counts.num_comparisons)*100.0);
  if(summary_data->counts.num_mislabeled > 0)
  {
    fprintf( outstream, "      avg. length..........................%.2lf bp\n",
             (double)summary_data->results.perfect_mislabeled.total_length /
             (double)summary_data->results.perfect_mislabeled.transcript_count );
    fprintf( outstream, "      avg. # refr exons....................%.2lf\n",
             (double)summary_data->results.perfect_mislabeled.refr_exon_count /
             (double)summary_data->results.perfect_mislabeled.transcript_count );
    fprintf( outstream, "      avg. # pred exons....................%.2lf\n",
             (double)summary_data->results.perfect_mislabeled.pred_exon_count /
             (double)summary_data->results.perfect_mislabeled.transcript_count );
    fprintf( outstream, "      avg. refr CDS length.................%.2lf aa\n",
             (double)summary_data->results.perfect_mislabeled.refr_cds_length /
             (double)summary_data->results.perfect_mislabeled.transcript_count );
    fprintf( outstream, "      avg. pred CDS length.................%.2lf aa\n",
             (double)summary_data->results.perfect_mislabeled.pred_cds_length /
             (double)summary_data->results.perfect_mislabeled.transcript_count );
  }
  fprintf( outstream, "    CDS structure matches..................%d (%.1f%%)\n",
           summary_data->counts.num_cds_match,
           ((float)summary_data->counts.num_cds_match/(float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.num_cds_match > 0)
  {
    fprintf( outstream, "      avg. length..........................%.2lf bp\n",
             (double)summary_data->results.cds_matches.total_length /
             (double)summary_data->results.cds_matches.transcript_count );
    fprintf( outstream, "      avg. # refr exons....................%.2lf\n",
             (double)summary_data->results.cds_matches.refr_exon_count /
             (double)summary_data->results.cds_matches.transcript_count );
    fprintf( outstream, "      avg. # pred exons....................%.2lf\n",
             (double)summary_data->results.cds_matches.pred_exon_count /
             (double)summary_data->results.cds_matches.transcript_count );
    fprintf( outstream, "      avg. refr CDS length.................%.2lf aa\n",
             (double)summary_data->results.cds_matches.refr_cds_length /
             (double)summary_data->results.cds_matches.transcript_count );
    fprintf( outstream, "      avg. pred CDS length.................%.2lf aa\n",
             (double)summary_data->results.cds_matches.pred_cds_length /
             (double)summary_data->results.cds_matches.transcript_count );
  }
  fprintf( outstream, "    exon structure matches.................%d (%.1f%%)\n",
           summary_data->counts.num_exon_match,
           ((float)summary_data->counts.num_exon_match/(float)summary_data->counts.num_comparisons)*100.0);
  if(summary_data->counts.num_exon_match > 0)
  {
    fprintf( outstream, "      avg. length..........................%.2lf bp\n",
             (double)summary_data->results.exon_matches.total_length /
             (double)summary_data->results.exon_matches.transcript_count );
    fprintf( outstream, "      avg. # refr exons....................%.2lf\n",
             (double)summary_data->results.exon_matches.refr_exon_count /
             (double)summary_data->results.exon_matches.transcript_count );
    fprintf( outstream, "      avg. # pred exons....................%.2lf\n",
             (double)summary_data->results.exon_matches.pred_exon_count /
             (double)summary_data->results.exon_matches.transcript_count );
    fprintf( outstream, "      avg. refr CDS length.................%.2lf aa\n",
             (double)summary_data->results.exon_matches.refr_cds_length /
             (double)summary_data->results.exon_matches.transcript_count );
    fprintf( outstream, "      avg. pred CDS length.................%.2lf aa\n",
             (double)summary_data->results.exon_matches.pred_cds_length /
             (double)summary_data->results.exon_matches.transcript_count );
  }
  fprintf( outstream, "    UTR structure matches..................%d (%.1f%%)\n",
           summary_data->counts.num_utr_match,
           ((float)summary_data->counts.num_utr_match/(float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.num_utr_match > 0)
  {
    fprintf( outstream, "      avg. length..........................%.2lf bp\n",
             (double)summary_data->results.utr_matches.total_length /
             (double)summary_data->results.utr_matches.transcript_count );
    fprintf( outstream, "      avg. # refr exons....................%.2lf\n",
             (double)summary_data->results.utr_matches.refr_exon_count /
             (double)summary_data->results.utr_matches.transcript_count );
    fprintf( outstream, "      avg. # pred exons....................%.2lf\n",
             (double)summary_data->results.utr_matches.pred_exon_count /
             (double)summary_data->results.utr_matches.transcript_count );
    fprintf( outstream, "      avg. refr CDS length.................%.2lf aa\n",
             (double)summary_data->results.utr_matches.refr_cds_length /
             (double)summary_data->results.utr_matches.transcript_count );
    fprintf( outstream, "      avg. pred CDS length.................%.2lf aa\n",
             (double)summary_data->results.utr_matches.pred_cds_length /
             (double)summary_data->results.utr_matches.transcript_count );
  }
  fprintf( outstream, "    non-matches............................%d (%.1f%%)\n",
           summary_data->counts.non_match,
           ((float)summary_data->counts.non_match / (float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.non_match > 0)
  {
    fprintf( outstream, "      avg. length..........................%.2lf bp\n",
             (double)summary_data->results.non_matches.total_length /
             (double)summary_data->results.non_matches.transcript_count );
    fprintf( outstream, "      avg. # refr exons....................%.2lf\n",
             (double)summary_data->results.non_matches.refr_exon_count /
             (double)summary_data->results.non_matches.transcript_count );
    fprintf( outstream, "      avg. # pred exons....................%.2lf\n",
             (double)summary_data->results.non_matches.pred_exon_count /
             (double)summary_data->results.non_matches.transcript_count );
    fprintf( outstream, "      avg. refr CDS length.................%.2lf aa\n",
             (double)summary_data->results.non_matches.refr_cds_length /
             (double)summary_data->results.non_matches.transcript_count );
    fprintf( outstream, "      avg. pred CDS length.................%.2lf aa\n",
             (double)summary_data->results.non_matches.pred_cds_length /
             (double)summary_data->results.non_matches.transcript_count );
  }
  fputs("\n", outstream);

  fprintf( outstream, "  CDS structure comparison\n" );
  fprintf( outstream, "    reference CDS segments.................%lu\n",
           summary_data->stats.cds_struc_stats.correct + summary_data->stats.cds_struc_stats.missing );
  fprintf( outstream, "      match prediction.....................%lu (%.1f%%)\n",
           summary_data->stats.cds_struc_stats.correct,
           ((float)summary_data->stats.cds_struc_stats.correct /(float)
           (summary_data->stats.cds_struc_stats.correct+summary_data->stats.cds_struc_stats.missing))*100 );
  fprintf( outstream, "      don't match prediction...............%lu (%.1f%%)\n",
           summary_data->stats.cds_struc_stats.missing,
           ((float)summary_data->stats.cds_struc_stats.missing/(float)
           (summary_data->stats.cds_struc_stats.correct+summary_data->stats.cds_struc_stats.missing))*100 );
  fprintf( outstream, "    prediction CDS segments................%lu\n",
           summary_data->stats.cds_struc_stats.correct + summary_data->stats.cds_struc_stats.wrong );
  fprintf( outstream, "      match reference......................%lu (%.1f%%)\n",
           summary_data->stats.cds_struc_stats.correct,
           ((float)summary_data->stats.cds_struc_stats.correct/(float)
           (summary_data->stats.cds_struc_stats.correct+summary_data->stats.cds_struc_stats.wrong))*100 );
  fprintf( outstream, "      don't match reference................%lu (%.1f%%)\n",
           summary_data->stats.cds_struc_stats.wrong,
           ((float)summary_data->stats.cds_struc_stats.wrong/(float)
           (summary_data->stats.cds_struc_stats.correct+summary_data->stats.cds_struc_stats.wrong))*100 );
  fprintf( outstream, "    Sensitivity............................%.3lf\n",
           summary_data->stats.cds_struc_stats.sn );
  fprintf( outstream, "    Specificity............................%.3lf\n",
           summary_data->stats.cds_struc_stats.sp );
  fprintf( outstream, "    F1 Score...............................%.3lf\n",
           summary_data->stats.cds_struc_stats.f1 );
  fprintf( outstream, "    Annotation edit distance...............%.3lf\n\n",
           summary_data->stats.cds_struc_stats.ed );

  fprintf( outstream, "  Exon structure comparison\n");
  fprintf( outstream, "    reference exons........................%lu\n",
           summary_data->stats.exon_struc_stats.correct + summary_data->stats.exon_struc_stats.missing );
  fprintf( outstream, "      match prediction.....................%lu (%.1f%%)\n",
           summary_data->stats.exon_struc_stats.correct,
           ((float)summary_data->stats.exon_struc_stats.correct/(float)
           (summary_data->stats.exon_struc_stats.correct+summary_data->stats.exon_struc_stats.missing))*100 );
  fprintf( outstream, "      don't match prediction...............%lu (%.1f%%)\n",
           summary_data->stats.exon_struc_stats.missing,
           ((float)summary_data->stats.exon_struc_stats.missing/(float)
           (summary_data->stats.exon_struc_stats.correct+summary_data->stats.exon_struc_stats.missing))*100 );
  fprintf( outstream, "    prediction exons.......................%lu\n",
           summary_data->stats.exon_struc_stats.correct + summary_data->stats.exon_struc_stats.wrong );
  fprintf( outstream, "      match reference......................%lu (%.1f%%)\n",
           summary_data->stats.exon_struc_stats.correct,
           ((float)summary_data->stats.exon_struc_stats.correct/(float)
           (summary_data->stats.exon_struc_stats.correct+summary_data->stats.exon_struc_stats.wrong))*100 );
  fprintf( outstream, "      don't match reference................%lu (%.1f%%)\n",
           summary_data->stats.exon_struc_stats.wrong,
           ((float)summary_data->stats.exon_struc_stats.wrong/(float)
           (summary_data->stats.exon_struc_stats.correct+summary_data->stats.exon_struc_stats.wrong))*100 );
  fprintf( outstream, "    Sensitivity............................%.3lf\n",
           summary_data->stats.exon_struc_stats.sn );
  fprintf( outstream, "    Specificity............................%.3lf\n",
           summary_data->stats.exon_struc_stats.sp );
  fprintf( outstream, "    F1 Score...............................%.3lf\n",
           summary_data->stats.exon_struc_stats.f1 );
  fprintf( outstream, "    Annotation edit distance...............%.3lf\n\n",
           summary_data->stats.exon_struc_stats.ed );

  fprintf( outstream, "  UTR structure comparison\n");
  fprintf( outstream, "    reference UTR segments.................%lu\n",
           summary_data->stats.utr_struc_stats.correct + summary_data->stats.utr_struc_stats.missing );
  if(summary_data->stats.utr_struc_stats.correct + summary_data->stats.utr_struc_stats.missing > 0)
  {
    fprintf( outstream, "      match prediction.....................%lu (%.1f%%)\n",
             summary_data->stats.utr_struc_stats.correct,
             ((float)summary_data->stats.utr_struc_stats.correct/(float)
             (summary_data->stats.utr_struc_stats.correct+summary_data->stats.utr_struc_stats.missing))*100 );
    fprintf( outstream, "      don't match prediction...............%lu (%.1f%%)\n",
             summary_data->stats.utr_struc_stats.missing,
             ((float)summary_data->stats.utr_struc_stats.missing/(float)
             (summary_data->stats.utr_struc_stats.correct+summary_data->stats.utr_struc_stats.missing))*100 );
  }
  fprintf( outstream, "    prediction UTR segments..................%lu\n",
           summary_data->stats.utr_struc_stats.correct + summary_data->stats.utr_struc_stats.wrong );
  if(summary_data->stats.utr_struc_stats.correct + summary_data->stats.utr_struc_stats.wrong > 0)
  {
    fprintf( outstream, "      match reference......................%lu (%.1f%%)\n",
             summary_data->stats.utr_struc_stats.correct,
             ((float)summary_data->stats.utr_struc_stats.correct/(float)
             (summary_data->stats.utr_struc_stats.correct+summary_data->stats.utr_struc_stats.wrong))*100 );
    fprintf( outstream, "      don't match reference................%lu (%.1f%%)\n",
             summary_data->stats.utr_struc_stats.wrong,
             ((float)summary_data->stats.utr_struc_stats.wrong/(float)
             (summary_data->stats.utr_struc_stats.correct+summary_data->stats.utr_struc_stats.wrong))*100 );
  }
  fprintf( outstream, "    Sensitivity............................%s\n",
           summary_data->stats.utr_struc_stats.sns );
  fprintf( outstream, "    Specificity............................%s\n",
           summary_data->stats.utr_struc_stats.sps );
  fprintf( outstream, "    F1 Score...............................%s\n",
           summary_data->stats.utr_struc_stats.f1s );
  fprintf( outstream, "    Annotation edit distance...............%s\n\n",
           summary_data->stats.utr_struc_stats.eds );

  fprintf( outstream, "  %-30s   %-10s   %-10s   %-10s\n",
           "Nucleotide-level comparison", "CDS", "UTRs", "Overall" );
  fprintf( outstream, "    %-30s %-10s   %-10s   %-.3lf\n",
           "Matching coefficient:", summary_data->stats.cds_nuc_stats.mcs,
           summary_data->stats.utr_nuc_stats.mcs, summary_data->stats.overall_identity);
  fprintf( outstream, "    %-30s %-10s   %-10s   %-10s\n",
           "Correlation coefficient:", summary_data->stats.cds_nuc_stats.ccs,
           summary_data->stats.utr_nuc_stats.ccs, "--");
  fprintf( outstream, "    %-30s %-10s   %-10s   %-10s\n", "Sensitivity:",
           summary_data->stats.cds_nuc_stats.sns, summary_data->stats.utr_nuc_stats.sns, "--");
  fprintf( outstream, "    %-30s %-10s   %-10s   %-10s\n", "Specificity:",
           summary_data->stats.cds_nuc_stats.sps, summary_data->stats.utr_nuc_stats.sps, "--");
  fprintf( outstream, "    %-30s %-10s   %-10s   %-10s\n", "F1 Score:",
           summary_data->stats.cds_nuc_stats.f1s, summary_data->stats.utr_nuc_stats.f1s, "--");
  fprintf( outstream, "    %-30s %-10s   %-10s   %-10s\n", "Annotation edit distance:",
           summary_data->stats.cds_nuc_stats.eds, summary_data->stats.utr_nuc_stats.eds, "--");

  fprintf(outstream, "\n\n\n");
}

void pe_print_summary_html( const char *start_time, int argc, char * const argv[],
                            GtStrArray *seqids, AgnSummaryData *summary_data,
                            AgnSummaryData *seq_summary_data, FILE *outstream, PeOptions *options )
{
  // Print header
  fputs( "<!doctype html>\n"
         "<html lang=\"en\">\n"
         "  <head>\n"
         "    <meta charset=\"utf-8\" />\n"
         "    <title>ParsEval Summary</title>\n"
         "    <link rel=\"stylesheet\" type=\"text/css\" href=\"parseval.css\" />\n"
         "    <script type=\"text/javascript\" language=\"javascript\" src=\"jquery.js\"></script>\n"
         "    <script type=\"text/javascript\" language=\"javascript\" src=\"jquery.dataTables.js\"></script>\n"
         "    <script type=\"text/javascript\">\n"
         "      $(document).ready(function() {\n"
         "        $('#seqlist').dataTable( {\n"
         "          \"sScrollY\": \"400px\",\n"
         "          \"bPaginate\": false,\n"
         "          \"bScrollCollapse\": true,\n"
         "          \"bSort\": false,\n"
         "          \"bFilter\": false,\n"
         "          \"bInfo\": false\n"
         "        });\n"
         "      } );\n"
         "    </script>\n"
         "  </head>\n", outstream );

  const char *refrlabel = options->refrfile;
  if(strcmp(options->refrlabel, "") != 0)
    refrlabel = options->refrlabel;
  const char *predlabel = options->predfile;
  if(strcmp(options->predlabel, "") != 0)
    predlabel = options->predlabel;
  // Print runtime summary
  fprintf( outstream,
           "  <body>\n"
           "    <div id=\"content\">\n"
           "      <h1>ParsEval Summary</h1>\n"
           "      <pre class=\"command\">\n"
           "Started:                %s\n"
           "Reference annotations:  %s\n"
           "Prediction annotations: %s\n"
           "Executing command:      ",
           start_time, refrlabel, predlabel );
  int x;
  for(x = 0; x < argc; x++)
  {
    fprintf(outstream, "%s ", argv[x]);
  }
  fprintf(outstream, "</pre>\n\n");

  // Print sequence summary
  if(!options->summary_only)
  {
    fputs( "      <h2>Sequences compared</h2>\n"
           "      <p class=\"indent\">Click on a sequence ID below to see comparison results for "
           "individual loci.</p>\n",
           outstream );
  }
  fputs( "      <table id=\"seqlist\" class=\"indent\">\n"
         "        <thead>\n"
         "          <tr>\n"
         "            <th>Sequence</th>\n"
         "            <th>Refr genes</th>\n"
         "            <th>Pred genes</th>\n"
         "            <th>Loci</th>\n"
         "          </tr>\n"
         "        </thead>\n"
         "        <tbody>\n", outstream);
  unsigned long i;
  for(i = 0; i < gt_str_array_size(seqids); i++)
  {
    const char *seqid = gt_str_array_get(seqids, i);
    if(options->summary_only || seq_summary_data[i].counts.num_loci == 0)
    {
      fprintf( outstream, "        <tr><td>%s</td><td>%lu</td><td>%lu</td><td>%lu</td></tr>\n",
               seqid, seq_summary_data[i].counts.num_loci, seq_summary_data[i].counts.refr_genes, seq_summary_data[i].counts.pred_genes );

      char cmd[512];
      sprintf(cmd, "rm -r %s/%s", options->outfilename, seqid);
      gt_assert(system(cmd) == 0);
    }
    else
      fprintf( outstream, "        <tr><td><a href=\"%s/index.html\">%s</a></td><td>%lu</td><td>%lu</td><td>%lu</td></tr>\n",
               seqid, seqid, seq_summary_data[i].counts.num_loci, seq_summary_data[i].counts.refr_genes, seq_summary_data[i].counts.pred_genes );
  }
  fputs("        </tbody>\n\n"
        "      </table>\n\n", outstream);

  // Print stats
  fprintf( outstream,
           "      <h2>Gene loci <span class=\"tooltip\">[?]<span class=\"tooltip_text\">If a gene "
           "annotation overlaps with another gene annotation, those annotations are associated "
           "with the same gene locus. See <a target=\"_blank\" "
           "href=\"http://parseval.sourceforge.net/about.html#locus_def\">"
           "this page</a> for a formal definition of a locus annotation.</span></span></h2>\n"
           "      <table class=\"table_normal\">\n"
           "        <tr><td>shared</td><td>%lu</td></tr>\n"
           "        <tr><td>unique to reference</td><td>%d</td></tr>\n"
           "        <tr><td>unique to prediction</td><td>%d</td></tr>\n"
           "        <tr><th class=\"right-align\">Total</th><td>%lu</td></tr>\n"
           "      </table>\n\n",
           summary_data->counts.num_loci - summary_data->counts.unique_refr - summary_data->counts.unique_pred,
           summary_data->counts.unique_refr, summary_data->counts.unique_pred, summary_data->counts.num_loci );

  fprintf( outstream,
           "      <h2>Reference annotations</h2>\n"
           "      <table class=\"table_normal\">\n"
           "        <tr><td>genes</td><td>%lu</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">average per locus</td>"
           "<td>%.3f</td></tr>\n"
           "        <tr><td>transcripts</td><td>%lu</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">average per locus</td>"
           "<td>%.3f</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">average per gene</td>"
           "<td>%.3f</td></tr>\n"
           "      </table>\n\n",
           summary_data->counts.refr_genes,
           (float)summary_data->counts.refr_genes / (float)summary_data->counts.num_loci,
           summary_data->counts.refr_transcripts,
           (float)summary_data->counts.refr_transcripts / (float)summary_data->counts.num_loci,
           (float)summary_data->counts.refr_transcripts / (float)summary_data->counts.refr_genes );

  fprintf( outstream,
           "      <h2>Prediction annotations</h2>\n"
           "      <table class=\"table_normal\">\n"
           "        <tr><td>genes</td><td>%lu</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">average per locus</td>"
           "<td>%.3f</td></tr>\n"
           "        <tr><td>transcripts</td><td>%lu</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">average per locus</td>"
           "<td>%.3f</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">average per gene</td>"
           "<td>%.3f</td></tr>\n"
           "      </table>\n\n",
           summary_data->counts.pred_genes,
           (float)summary_data->counts.pred_genes / (float)summary_data->counts.num_loci,
           summary_data->counts.pred_transcripts,
           (float)summary_data->counts.pred_transcripts / (float)summary_data->counts.num_loci,
           (float)summary_data->counts.pred_transcripts / (float)summary_data->counts.pred_genes );

  fputs( "      <h2>Comparisons</h2>\n"
         "      <table class=\"comparisons\">\n", outstream );
  fprintf( outstream, "<tr><th>Total comparisons</th><th>%u</th></tr>\n",
           summary_data->counts.num_comparisons );
  fprintf( outstream,
           "        <tr><td>perfect matches <span class=\"tooltip\"><span class=\"small_tooltip\">"
           "[?]</span><span class=\"tooltip_text\">Prediction transcripts (exons, coding sequences,"
           "and UTRs) line up perfectly with reference transcripts.</span></span></td>"
           "<td>%d (%.1f%%)</tr>\n",
           summary_data->counts.num_perfect,
           ((float)summary_data->counts.num_perfect / (float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.num_perfect > 0)
  {
    fprintf( outstream,
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average length</td>"
             "<td>%.2lf bp</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # refr exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # pred exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average refr CDS length"
             "</td><td>%.2lf aa</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average pred CDS length"
             "</td><td>%.2lf aa</td></tr>\n",
             (double)summary_data->results.perfect_matches.total_length /
             (double)summary_data->results.perfect_matches.transcript_count,
             (double)summary_data->results.perfect_matches.refr_exon_count /
             (double)summary_data->results.perfect_matches.transcript_count,
             (double)summary_data->results.perfect_matches.pred_exon_count /
             (double)summary_data->results.perfect_matches.transcript_count,
             (double)summary_data->results.perfect_matches.refr_cds_length /
             (double)summary_data->results.perfect_matches.transcript_count,
             (double)summary_data->results.perfect_matches.pred_cds_length /
             (double)summary_data->results.perfect_matches.transcript_count );
  }
  fprintf( outstream,
           "        <tr><td>perfect matches with mislabeled UTRs <span class=\"tooltip\">"
           "<span class=\"small_tooltip\">[?]</span><span class=\"tooltip_text\">5'/3' orientation"
           " of UTRs is reversed between reference and prediction, but a perfect match in all other"
           " aspects.</span></span></td><td>%d (%.1f%%)</tr>\n",
           summary_data->counts.num_mislabeled,
           ((float)summary_data->counts.num_mislabeled / (float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.num_mislabeled > 0)
  {
    fprintf( outstream,
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average length</td>"
             "<td>%.2lf bp</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # refr exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # pred exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average refr CDS length"
             "</td><td>%.2lf aa</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average pred CDS length"
             "</td><td>%.2lf aa</td></tr>\n",
             (double)summary_data->results.perfect_mislabeled.total_length /
             (double)summary_data->results.perfect_mislabeled.transcript_count,
             (double)summary_data->results.perfect_mislabeled.refr_exon_count /
             (double)summary_data->results.perfect_mislabeled.transcript_count,
             (double)summary_data->results.perfect_mislabeled.pred_exon_count /
             (double)summary_data->results.perfect_mislabeled.transcript_count,
             (double)summary_data->results.perfect_mislabeled.refr_cds_length /
             (double)summary_data->results.perfect_mislabeled.transcript_count,
             (double)summary_data->results.perfect_mislabeled.pred_cds_length /
             (double)summary_data->results.perfect_mislabeled.transcript_count );
  }
  fprintf( outstream,
           "        <tr><td>CDS structure matches <span class=\"tooltip\">"
           "<span class=\"small_tooltip\">[?]</span><span class=\"tooltip_text\">Not a perfect"
           " match, but prediction coding sequence(s) line up perfectly with reference coding"
           " sequence(s).</span></span></td><td>%d (%.1f%%)</tr>\n",
           summary_data->counts.num_cds_match,
           ((float)summary_data->counts.num_cds_match / (float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.num_cds_match > 0)
  {
    fprintf( outstream,
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average length</td>"
             "<td>%.2lf bp</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # refr exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # pred exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average refr CDS length"
             "</td><td>%.2lf aa</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average pred CDS length"
             "</td><td>%.2lf aa</td></tr>\n",
             (double)summary_data->results.cds_matches.total_length /
             (double)summary_data->results.cds_matches.transcript_count,
             (double)summary_data->results.cds_matches.refr_exon_count /
             (double)summary_data->results.cds_matches.transcript_count,
             (double)summary_data->results.cds_matches.pred_exon_count /
             (double)summary_data->results.cds_matches.transcript_count,
             (double)summary_data->results.cds_matches.refr_cds_length /
             (double)summary_data->results.cds_matches.transcript_count,
             (double)summary_data->results.cds_matches.pred_cds_length /
             (double)summary_data->results.cds_matches.transcript_count );
  }
  fprintf( outstream,
           "        <tr><td>exon structure matches <span class=\"tooltip\">"
           "<span class=\"small_tooltip\">[?]</span><span class=\"tooltip_text\">Not a perfect"
           " match or CDS match, but prediction exon structure is identical to reference exon"
           " structure.</span></span></td><td>%d (%.1f%%)</tr>\n",
           summary_data->counts.num_exon_match,
           ((float)summary_data->counts.num_exon_match / (float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.num_exon_match > 0)
  {
    fprintf( outstream,
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average length</td>"
             "<td>%.2lf bp</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # refr exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # pred exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average refr CDS length"
             "</td><td>%.2lf aa</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average pred CDS length"
             "</td><td>%.2lf aa</td></tr>\n",
             (double)summary_data->results.exon_matches.total_length /
             (double)summary_data->results.exon_matches.transcript_count,
             (double)summary_data->results.exon_matches.refr_exon_count /
             (double)summary_data->results.exon_matches.transcript_count,
             (double)summary_data->results.exon_matches.pred_exon_count /
             (double)summary_data->results.exon_matches.transcript_count,
             (double)summary_data->results.exon_matches.refr_cds_length /
             (double)summary_data->results.exon_matches.transcript_count,
             (double)summary_data->results.exon_matches.pred_cds_length /
             (double)summary_data->results.exon_matches.transcript_count );
  }
  fprintf( outstream,
        "        <tr><td>UTR structure matches <span class=\"tooltip\">"
        "<span class=\"small_tooltip\">[?]</span><span class=\"tooltip_text\">Not a perfect match,"
        " CDS match, or exon structure match, but prediction UTRs line up perfectly with reference"
        " UTRs.</span></span></td><td>%d (%.1f%%)</tr>\n",
           summary_data->counts.num_utr_match,
           ((float)summary_data->counts.num_utr_match / (float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.num_utr_match > 0)
  {
    fprintf( outstream,
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average length</td>"
             "<td>%.2lf bp</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # refr exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # pred exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average refr CDS length"
             "</td><td>%.2lf aa</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average pred CDS length"
             "</td><td>%.2lf aa</td></tr>\n",
             (double)summary_data->results.utr_matches.total_length /
             (double)summary_data->results.utr_matches.transcript_count,
             (double)summary_data->results.utr_matches.refr_exon_count /
             (double)summary_data->results.utr_matches.transcript_count,
             (double)summary_data->results.utr_matches.pred_exon_count /
             (double)summary_data->results.utr_matches.transcript_count,
             (double)summary_data->results.utr_matches.refr_cds_length /
             (double)summary_data->results.utr_matches.transcript_count,
             (double)summary_data->results.utr_matches.pred_cds_length /
             (double)summary_data->results.utr_matches.transcript_count );
  }
  fprintf( outstream, "        <tr><td>non-matches</td><td>%d (%.1f%%)</tr>\n",
           summary_data->counts.non_match,
           ((float)summary_data->counts.non_match / (float)summary_data->counts.num_comparisons)*100.0 );
  if(summary_data->counts.non_match > 0)
  {
    fprintf( outstream,
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average length</td>"
             "<td>%.2lf bp</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # refr exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average # pred exons</td>"
             "<td>%.2lf</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average refr CDS length"
             "</td><td>%.2lf aa</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">average pred CDS length"
             "</td><td>%.2lf aa</td></tr>\n",
             (double)summary_data->results.non_matches.total_length /
             (double)summary_data->results.non_matches.transcript_count,
             (double)summary_data->results.non_matches.refr_exon_count /
             (double)summary_data->results.non_matches.transcript_count,
             (double)summary_data->results.non_matches.pred_exon_count /
             (double)summary_data->results.non_matches.transcript_count,
             (double)summary_data->results.non_matches.refr_cds_length /
             (double)summary_data->results.non_matches.transcript_count,
             (double)summary_data->results.non_matches.pred_cds_length /
             (double)summary_data->results.non_matches.transcript_count );
  }
  fputs( "      </table>\n\n", outstream );

  fprintf( outstream,
           "      <h2 class=\"bottomspace\">Comparison statistics</h2>\n"
           "      <h3>CDS structure comparison</h3>\n"
           "      <table class=\"table_normal table_extra_indent\">\n"
           "        <tr><td>reference CDS segments</td><td>%lu</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction</td>"
           "<td>%lu (%.1f%%)</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">don't match prediction</td>"
           "<td>%lu (%.1f%%)</td></tr>\n"
           "        <tr><td>prediction CDS segments</td><td>%lu</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction</td>"
           "<td>%lu (%.1f%%)</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">don't match prediction</td>"
           "<td>%lu (%.1f%%)</td></tr>\n"
           "        <tr><td>sensitivity</td><td>%.3f</td></tr>\n"
           "        <tr><td>specificity</td><td>%.3f</td></tr>\n"
           "        <tr><td>F1 score</td><td>%.3f</td></tr>\n"
           "        <tr><td>annotation edit distance</td><td>%.3f</td></tr>\n"
           "      </table>\n\n",
           summary_data->stats.cds_struc_stats.correct + summary_data->stats.cds_struc_stats.missing,
           summary_data->stats.cds_struc_stats.correct,
           ((float)summary_data->stats.cds_struc_stats.correct /(float)
           (summary_data->stats.cds_struc_stats.correct+summary_data->stats.cds_struc_stats.missing))*100,
           summary_data->stats.cds_struc_stats.missing,
           ((float)summary_data->stats.cds_struc_stats.missing/(float)
           (summary_data->stats.cds_struc_stats.correct+summary_data->stats.cds_struc_stats.missing))*100,
           summary_data->stats.cds_struc_stats.correct + summary_data->stats.cds_struc_stats.wrong,
           summary_data->stats.cds_struc_stats.correct,
           ((float)summary_data->stats.cds_struc_stats.correct/(float)
           (summary_data->stats.cds_struc_stats.correct+summary_data->stats.cds_struc_stats.wrong))*100,
           summary_data->stats.cds_struc_stats.wrong,
           ((float)summary_data->stats.cds_struc_stats.wrong/(float)
           (summary_data->stats.cds_struc_stats.correct+summary_data->stats.cds_struc_stats.wrong))*100,
           summary_data->stats.cds_struc_stats.sn, summary_data->stats.cds_struc_stats.sp,
           summary_data->stats.cds_struc_stats.f1, summary_data->stats.cds_struc_stats.ed );

  fprintf( outstream,
           "      <h3>Exon structure comparison</h3>\n"
           "      <table class=\"table_normal table_extra_indent\">\n"
           "        <tr><td>reference exons</td><td>%lu</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction</td>"
           "<td>%lu (%.1f%%)</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">don't match prediction</td>"
           "<td>%lu (%.1f%%)</td></tr>\n"
           "        <tr><td>prediction exons</td><td>%lu</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction</td>"
           "<td>%lu (%.1f%%)</td></tr>\n"
           "        <tr class=\"cell_small\"><td class=\"cell_indent\">don't match prediction</td>"
           "<td>%lu (%.1f%%)</td></tr>\n"
           "        <tr><td>sensitivity</td><td>%.3f</td></tr>\n"
           "        <tr><td>specificity</td><td>%.3f</td></tr>\n"
           "        <tr><td>F1 score</td><td>%.3f</td></tr>\n"
           "        <tr><td>annotation edit distance</td><td>%.3f</td></tr>\n"
           "      </table>\n\n",
           summary_data->stats.exon_struc_stats.correct + summary_data->stats.exon_struc_stats.missing,
           summary_data->stats.exon_struc_stats.correct,
           ((float)summary_data->stats.exon_struc_stats.correct /(float)
           (summary_data->stats.exon_struc_stats.correct+summary_data->stats.exon_struc_stats.missing))*100,
           summary_data->stats.exon_struc_stats.missing,
           ((float)summary_data->stats.exon_struc_stats.missing/(float)
           (summary_data->stats.exon_struc_stats.correct+summary_data->stats.exon_struc_stats.missing))*100,
           summary_data->stats.exon_struc_stats.correct + summary_data->stats.exon_struc_stats.wrong,
           summary_data->stats.exon_struc_stats.correct,
           ((float)summary_data->stats.exon_struc_stats.correct/(float)
           (summary_data->stats.exon_struc_stats.correct+summary_data->stats.exon_struc_stats.wrong))*100,
           summary_data->stats.exon_struc_stats.wrong,
           ((float)summary_data->stats.exon_struc_stats.wrong/(float)
           (summary_data->stats.exon_struc_stats.correct+summary_data->stats.exon_struc_stats.wrong))*100,
           summary_data->stats.exon_struc_stats.sn, summary_data->stats.exon_struc_stats.sp,
           summary_data->stats.exon_struc_stats.f1, summary_data->stats.exon_struc_stats.ed );

  fprintf( outstream,
           "      <h3>UTR structure comparison</h3>\n"
           "      <table class=\"table_normal table_extra_indent\">\n"
           "        <tr><td>reference UTR segments</td><td>%lu</td></tr>\n",
           summary_data->stats.utr_struc_stats.correct + summary_data->stats.utr_struc_stats.missing );
  if(summary_data->stats.utr_struc_stats.correct + summary_data->stats.utr_struc_stats.missing > 0)
  {
    fprintf( outstream,
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction</td>"
             "<td>%lu (%.1f%%)</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">don't match prediction</td>"
             "<td>%lu (%.1f%%)</td></tr>\n",
             summary_data->stats.utr_struc_stats.correct,
             ((float)summary_data->stats.utr_struc_stats.correct /(float)
             (summary_data->stats.utr_struc_stats.correct+summary_data->stats.utr_struc_stats.missing))*100,
             summary_data->stats.utr_struc_stats.missing,
             ((float)summary_data->stats.utr_struc_stats.missing/(float)
             (summary_data->stats.utr_struc_stats.correct+summary_data->stats.utr_struc_stats.missing))*100 );
  }
  fprintf( outstream, "        <tr><td>prediction UTR segments</td><td>%lu</td></tr>\n",
           summary_data->stats.utr_struc_stats.correct + summary_data->stats.utr_struc_stats.wrong );
  if(summary_data->stats.utr_struc_stats.correct + summary_data->stats.utr_struc_stats.wrong > 0)
  {
    fprintf( outstream,
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">match prediction</td>"
             "<td>%lu (%.1f%%)</td></tr>\n"
             "        <tr class=\"cell_small\"><td class=\"cell_indent\">don't match prediction</td>"
             "<td>%lu (%.1f%%)</td></tr>\n",
           summary_data->stats.utr_struc_stats.correct,
           ((float)summary_data->stats.utr_struc_stats.correct/(float)
           (summary_data->stats.utr_struc_stats.correct+summary_data->stats.utr_struc_stats.wrong))*100,
           summary_data->stats.utr_struc_stats.wrong,
           ((float)summary_data->stats.utr_struc_stats.wrong/(float)
           (summary_data->stats.utr_struc_stats.correct+summary_data->stats.utr_struc_stats.wrong))*100 );
  }

  fprintf( outstream,
           "        <tr><td>sensitivity</td><td>%s</td></tr>\n"
           "        <tr><td>specificity</td><td>%s</td></tr>\n"
           "        <tr><td>F1 score</td><td>%s</td></tr>\n"
           "        <tr><td>annotation edit distance</td><td>%s</td></tr>\n"
           "      </table>\n\n",
           summary_data->stats.utr_struc_stats.sns,
           summary_data->stats.utr_struc_stats.sps,
           summary_data->stats.utr_struc_stats.f1s,
           summary_data->stats.utr_struc_stats.eds );

  fprintf( outstream,
           "      <h3>Nucleotide-level comparison</h3>\n"
           "      <table class=\"table_wide table_extra_indent\">\n"
           "        <tr><th>&nbsp;</th><th>CDS</th><th>UTRs</th><th>Overall</th></tr>\n"
           "        <tr><th class=\"left-align\">matching coefficient</th><td>%s</td>"
           "<td>%s</td><td>%.3lf</td></tr>\n"
           "        <tr><th class=\"left-align\">correlation coefficient</th><td>%s</td>"
           "<td>%s</td><td>--</td></tr>\n"
           "        <tr><th class=\"left-align\">sensitivity</th><td>%s</td><td>%s</td>"
           "<td>--</td></tr>\n"
           "        <tr><th class=\"left-align\">specificity</th><td>%s</td><td>%s</td>"
           "<td>--</td></tr>\n"
           "        <tr><th class=\"left-align\">F1 score</th><td>%s</td><td>%s</td>"
           "<td>--</td></tr>\n"
           "        <tr><th class=\"left-align\">annotation edit distance</th><td>%s</td><td>%s</td>"
           "<td>--</td></tr>\n"
           "      </table>\n\n",
           summary_data->stats.cds_nuc_stats.mcs, summary_data->stats.utr_nuc_stats.mcs,
           summary_data->stats.overall_identity, summary_data->stats.cds_nuc_stats.ccs,
           summary_data->stats.utr_nuc_stats.ccs, summary_data->stats.cds_nuc_stats.sns,
           summary_data->stats.utr_nuc_stats.sns, summary_data->stats.cds_nuc_stats.sps,
           summary_data->stats.utr_nuc_stats.sps, summary_data->stats.cds_nuc_stats.f1s,
           summary_data->stats.utr_nuc_stats.f1s, summary_data->stats.cds_nuc_stats.eds,
           summary_data->stats.utr_nuc_stats.eds );

  pe_print_html_footer(outstream);

  fputs( "    </div>\n"
         "  </body>\n"
         "</html>\n",
         outstream );
}

int pe_track_order(const char *s1, const char *s2, void *data)
{
  if(strstr(s1, "Reference") == NULL)
    return 1;
  
  return -1;
}