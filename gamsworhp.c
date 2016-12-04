/* simple main program that loads a GAMS control file and
 * does some inspection on the model instance
 */

#include <stdio.h>
#include <stdlib.h>

#include "gmomcc.h"
#include "gevmcc.h"

#include "worhp.h"

int main(int argc, char** argv)
{
   gmoHandle_t gmo = NULL;
   gevHandle_t gev = NULL;
   int rc = EXIT_FAILURE;
   char buffer[1024];

   if( argc < 2 )
   {
      printf("usage: %s <cntrlfile>\n", argv[0]);
      return 1;
   }

   /* initialize GMO and GEV libraries */
   if( !gmoCreate(&gmo, buffer, sizeof(buffer)) || !gevCreate(&gev, buffer, sizeof(buffer)) )
   {
      fprintf(stderr, "%s\n", buffer);
      goto TERMINATE;
   }

   /* load control file */
   if( gevInitEnvironmentLegacy(gev, argv[1]) )
   {
      fprintf(stderr, "Could not load control file %s\n", argv[1]);
      goto TERMINATE;
   }

   /* let gmo know about gev */
   if( gmoRegisterEnvironment(gmo, gev, buffer) )
   {
      fprintf(stderr, "Error registering GAMS Environment: %s\n", buffer);
      goto TERMINATE;
   }

   /* load instance data */
   if( gmoLoadDataLegacy(gmo, buffer) )
   {
      fprintf(stderr, "Could not load model data.\n");
      goto TERMINATE;
   }

   sprintf(buffer, "This is WORHP %d.%d." WORHP_PATCH ".", WORHP_MAJOR, WORHP_MINOR);
   gevLogStat(gev, buffer);

   /* reformulate objective variable out of model, if possible */
   gmoObjStyleSet(gmo, gmoObjType_Fun);

   /* print instance size */
   printf("Number of variable: %d\n", gmoN(gmo));
   printf("Number of constraints: %d\n", gmoM(gmo));

   /* evaluate objective and constraints */
   {
      int i, j;
      double* x;
      double val;
      int numerr;
      char name[GMS_SSSIZE];

      /* get initial point */
      x = (double*) malloc(gmoN(gmo) * sizeof(double));
      gmoGetVarL(gmo, x);

      /* print (max) first 5 components of x */
      printf("Evaluate at initial point (first 5 components):\n");
      for( i = 0; i < gmoN(gmo) && i < 5; ++i )
      {
         gmoGetVarNameOne(gmo, i, name);
         printf("\t%-30s = %g\n", name, x[i]);
      }

      /* print objective value */
      gmoEvalFuncObj(gmo, x, &val, &numerr);
      if( numerr == 0 )
         printf("Value objective: %g\n", val);
      else
         printf("Objective could not be evaluated. Error code: %d\n", numerr);

      /* print (max) first 5 constraint activities */
      for( j = 0; j < gmoM(gmo) && j < 5; ++j )
      {
         gmoEvalFunc(gmo, j, x, &val, &numerr);
         gmoGetEquNameOne(gmo, j, name);
         if( numerr == 0 )
            printf("Activity constraint %2d [%-30s]: %g\n", j, name, val);
         else
            printf("Activity constraint %2d [%-30s]: n/a, error code %d\n", j, name, numerr);
      }

      /* set primal solution */
      for( i = 0; i < gmoN(gmo); ++i )
         x[i] = 42.0;
      gmoSetSolutionPrimal(gmo, x);
   }

   /* pass the solution info back to GAMS */
   gmoSolveStatSet(gmo, gmoSolveStat_Solver);
   gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
   gmoUnloadSolutionLegacy(gmo);

   rc = EXIT_SUCCESS;

TERMINATE:
   if( gmo != NULL )
      gmoFree(&gmo);
   if( gev != NULL )
      gevFree(&gev);

   return rc;
}

