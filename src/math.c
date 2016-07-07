/*
 * This file is part of nmealib.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <math.h>
#include <nmealib/math.h>

double nmeaMathDegreeToRadian(const double val) {
  return (val * NMEALIB_PI180);
}

double nmeaMathRadianToDegree(const double val) {
  return (val / NMEALIB_PI180);
}

double nmeaMathNdegToDegree(const double val) {
  double deg;
  double fra_part = modf(val / 100.0, &deg);
  return (deg + ((fra_part * 100.0) / 60.0));
}

double nmeaMathDegreeToNdeg(const double val) {
  double deg;
  double fra_part = modf(val, &deg);
  return ((deg * 100.0) + (fra_part * 60.0));
}

double nmeaMathNdegToRadian(const double val) {
  return nmeaMathDegreeToRadian(nmeaMathNdegToDegree(val));
}

double nmeaMathRadianToNdeg(const double val) {
  return nmeaMathDegreeToNdeg(nmeaMathRadianToDegree(val));
}

double nmeaMathPdopCalculate(const double hdop, const double vdop) {
  return sqrt(pow(hdop, 2) + pow(vdop, 2));
}

double nmeaMathDopToMeters(const double dop) {
  return (dop * NMEALIB_DOP_FACTOR);
}

double nmeaMathMetersToDop(const double meters) {
  return (meters / NMEALIB_DOP_FACTOR);
}

double nmeaMathDistance(const NmeaPosition *from_pos, const NmeaPosition *to_pos) {
  return ((double) NMEALIB_EARTHRADIUS_M)
      * acos(
          sin(to_pos->lat) * sin(from_pos->lat)
              + cos(to_pos->lat) * cos(from_pos->lat) * cos(to_pos->lon - from_pos->lon));
}

double nmeaMathDistanceEllipsoid(const NmeaPosition *from_pos, const NmeaPosition *to_pos, double *from_azimuth, double *to_azimuth) {
  /* All variables */
  double f, a, b, sqr_a, sqr_b;
  double L, phi1, phi2, U1, U2, sin_U1, sin_U2, cos_U1, cos_U2;
  double sigma, sin_sigma, cos_sigma, cos_2_sigmam, sqr_cos_2_sigmam, sqr_cos_alpha, lambda, sin_lambda, cos_lambda,
      delta_lambda;
  int remaining_steps;
  double sqr_u, A, B, delta_sigma, lambda_prev;

  /* Check input */
  assert(from_pos != 0);
  assert(to_pos != 0);

  if ((from_pos->lat == to_pos->lat) && (from_pos->lon == to_pos->lon)) { /* Identical points */
    if (from_azimuth != 0)
      *from_azimuth = 0;
    if (to_azimuth != 0)
      *to_azimuth = 0;
    return 0;
  } /* Identical points */

  /* Earth geometry */
  f = NMEALIB_EARTH_FLATTENING;
  a = NMEALIB_EARTH_SEMIMAJORAXIS_M;
  b = (1 - f) * a;
  sqr_a = a * a;
  sqr_b = b * b;

  /* Calculation */
  L = to_pos->lon - from_pos->lon;
  phi1 = from_pos->lat;
  phi2 = to_pos->lat;
  U1 = atan((1 - f) * tan(phi1));
  U2 = atan((1 - f) * tan(phi2));
  sin_U1 = sin(U1);
  sin_U2 = sin(U2);
  cos_U1 = cos(U1);
  cos_U2 = cos(U2);

  /* Initialize iteration */
  sigma = 0;
  sin_sigma = sin(sigma);
  cos_sigma = cos(sigma);
  cos_2_sigmam = 0;
  sqr_cos_2_sigmam = cos_2_sigmam * cos_2_sigmam;
  sqr_cos_alpha = 0;
  lambda = L;
  sin_lambda = sin(lambda);
  cos_lambda = cos(lambda);
  lambda_prev = (double) 2.0 * (double) NMEALIB_PI;
  delta_lambda = lambda_prev - lambda;
  if (delta_lambda < 0)
    delta_lambda = -delta_lambda;
  remaining_steps = 20;

  while ((delta_lambda > 1e-12) && (remaining_steps > 0)) { /* Iterate */
    /* Variables */
    double tmp1, tmp2, sin_alpha, cos_alpha, C;

    /* Calculation */
    tmp1 = cos_U2 * sin_lambda;
    tmp2 = cos_U1 * sin_U2 - sin_U1 * cos_U2 * cos_lambda;
    sin_sigma = sqrt(tmp1 * tmp1 + tmp2 * tmp2);
    cos_sigma = sin_U1 * sin_U2 + cos_U1 * cos_U2 * cos_lambda;
    sin_alpha = cos_U1 * cos_U2 * sin_lambda / sin_sigma;
    cos_alpha = cos(asin(sin_alpha));
    sqr_cos_alpha = cos_alpha * cos_alpha;
    cos_2_sigmam = cos_sigma - 2 * sin_U1 * sin_U2 / sqr_cos_alpha;
    sqr_cos_2_sigmam = cos_2_sigmam * cos_2_sigmam;
    C = f / 16 * sqr_cos_alpha * (4 + f * (4 - 3 * sqr_cos_alpha));
    lambda_prev = lambda;
    sigma = asin(sin_sigma);
    lambda = L
        + (1 - C) * f * sin_alpha
            * (sigma + C * sin_sigma * (cos_2_sigmam + C * cos_sigma * (-1 + 2 * sqr_cos_2_sigmam)));
    delta_lambda = lambda_prev - lambda;
    if (delta_lambda < 0)
      delta_lambda = -delta_lambda;
    sin_lambda = sin(lambda);
    cos_lambda = cos(lambda);
    remaining_steps--;
  } /* Iterate */

  /* More calculation  */
  sqr_u = sqr_cos_alpha * (sqr_a - sqr_b) / sqr_b;
  A = 1 + sqr_u / 16384 * (4096 + sqr_u * (-768 + sqr_u * (320 - 175 * sqr_u)));
  B = sqr_u / 1024 * (256 + sqr_u * (-128 + sqr_u * (74 - 47 * sqr_u)));
  delta_sigma = B * sin_sigma
      * (cos_2_sigmam
          + B / 4
              * (cos_sigma * (-1 + 2 * sqr_cos_2_sigmam)
                  - B / 6 * cos_2_sigmam * (-3 + 4 * sin_sigma * sin_sigma) * (-3 + 4 * sqr_cos_2_sigmam)));

  /* Calculate result */
  if (from_azimuth != 0) {
    double tan_alpha_1 = cos_U2 * sin_lambda / (cos_U1 * sin_U2 - sin_U1 * cos_U2 * cos_lambda);
    *from_azimuth = atan(tan_alpha_1);
  }
  if (to_azimuth != 0) {
    double tan_alpha_2 = cos_U1 * sin_lambda / (-sin_U1 * cos_U2 + cos_U1 * sin_U2 * cos_lambda);
    *to_azimuth = atan(tan_alpha_2);
  }

  return b * A * (sigma - delta_sigma);
}

int nmeaMathMoveFlat(const NmeaPosition *start_pos, NmeaPosition *end_pos, double azimuth, double distance) {
  NmeaPosition p1 = *start_pos;
  int RetVal = 1;

  distance /= NMEALIB_EARTHRADIUS_KM; /* Angular distance covered on earth's surface */
  azimuth = nmeaMathDegreeToRadian(azimuth);

  end_pos->lat = asin(sin(p1.lat) * cos(distance) + cos(p1.lat) * sin(distance) * cos(azimuth));
  end_pos->lon = p1.lon
      + atan2(sin(azimuth) * sin(distance) * cos(p1.lat), cos(distance) - sin(p1.lat) * sin(end_pos->lat));

  if (isnan(end_pos->lat) || isnan(end_pos->lon)) {
    end_pos->lat = 0;
    end_pos->lon = 0;
    RetVal = 0;
  }

  return RetVal;
}

int nmeaMathMoveFlatEllipsoid(const NmeaPosition *start_pos, NmeaPosition *end_pos, double azimuth, double distance,
    double *end_azimuth) {
  /* Variables */
  double f, a, b, sqr_a, sqr_b;
  double phi1, tan_U1, sin_U1, cos_U1, s, alpha1, sin_alpha1, cos_alpha1;
  double sigma1, sin_alpha, sqr_cos_alpha, sqr_u, A, B;
  double sigma_initial, sigma, sigma_prev, sin_sigma, cos_sigma, cos_2_sigmam, sqr_cos_2_sigmam, delta_sigma;
  int remaining_steps;
  double tmp1, phi2, lambda, C, L;

  /* Check input */
  assert(start_pos != 0);
  assert(end_pos != 0);

  if (fabs(distance) < 1e-12) { /* No move */
    *end_pos = *start_pos;
    if (end_azimuth != 0)
      *end_azimuth = azimuth;
    return !(isnan(end_pos->lat) || isnan(end_pos->lon));
  } /* No move */

  /* Earth geometry */
  f = NMEALIB_EARTH_FLATTENING;
  a = NMEALIB_EARTH_SEMIMAJORAXIS_M;
  b = (1 - f) * a;
  sqr_a = a * a;
  sqr_b = b * b;

  /* Calculation */
  phi1 = start_pos->lat;
  tan_U1 = (1 - f) * tan(phi1);
  cos_U1 = 1 / sqrt(1 + tan_U1 * tan_U1);
  sin_U1 = tan_U1 * cos_U1;
  s = distance;
  alpha1 = azimuth;
  sin_alpha1 = sin(alpha1);
  cos_alpha1 = cos(alpha1);
  sigma1 = atan2(tan_U1, cos_alpha1);
  sin_alpha = cos_U1 * sin_alpha1;
  sqr_cos_alpha = 1 - sin_alpha * sin_alpha;
  sqr_u = sqr_cos_alpha * (sqr_a - sqr_b) / sqr_b;
  A = 1 + sqr_u / 16384 * (4096 + sqr_u * (-768 + sqr_u * (320 - 175 * sqr_u)));
  B = sqr_u / 1024 * (256 + sqr_u * (-128 + sqr_u * (74 - 47 * sqr_u)));

  /* Initialize iteration */
  sigma_initial = s / (b * A);
  sigma = sigma_initial;
  sin_sigma = sin(sigma);
  cos_sigma = cos(sigma);
  cos_2_sigmam = cos(2 * sigma1 + sigma);
  sqr_cos_2_sigmam = cos_2_sigmam * cos_2_sigmam;
  delta_sigma = 0;
  sigma_prev = 2 * NMEALIB_PI;
  remaining_steps = 20;

  while ((fabs(sigma - sigma_prev) > 1e-12) && (remaining_steps > 0)) { /* Iterate */
    cos_2_sigmam = cos(2 * sigma1 + sigma);
    sqr_cos_2_sigmam = cos_2_sigmam * cos_2_sigmam;
    sin_sigma = sin(sigma);
    cos_sigma = cos(sigma);
    delta_sigma = B * sin_sigma
        * (cos_2_sigmam
            + B / 4
                * (cos_sigma * (-1 + 2 * sqr_cos_2_sigmam)
                    - B / 6 * cos_2_sigmam * (-3 + 4 * sin_sigma * sin_sigma) * (-3 + 4 * sqr_cos_2_sigmam)));
    sigma_prev = sigma;
    sigma = sigma_initial + delta_sigma;
    remaining_steps--;
  } /* Iterate */

  /* Calculate result */
  tmp1 = (sin_U1 * sin_sigma - cos_U1 * cos_sigma * cos_alpha1);
  phi2 = atan2(sin_U1 * cos_sigma + cos_U1 * sin_sigma * cos_alpha1,
      (1 - f) * sqrt(sin_alpha * sin_alpha + tmp1 * tmp1));
  lambda = atan2(sin_sigma * sin_alpha1, cos_U1 * cos_sigma - sin_U1 * sin_sigma * cos_alpha1);
  C = f / 16 * sqr_cos_alpha * (4 + f * (4 - 3 * sqr_cos_alpha));
  L = lambda
      - (1 - C) * f * sin_alpha
          * (sigma + C * sin_sigma * (cos_2_sigmam + C * cos_sigma * (-1 + 2 * sqr_cos_2_sigmam)));

  /* Result */
  end_pos->lon = start_pos->lon + L;
  end_pos->lat = phi2;
  if (end_azimuth != 0) {
    *end_azimuth = atan2(sin_alpha, -sin_U1 * sin_sigma + cos_U1 * cos_sigma * cos_alpha1);
  }
  return !(isnan(end_pos->lat) || isnan(end_pos->lon));
}

void nmeaMathInfoToPosition(const NmeaInfo *info, NmeaPosition *pos) {
  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_LAT))
    pos->lat = nmeaMathNdegToRadian(info->lat);
  else
    pos->lat = NMEALIB_LATITUDE_DEFAULT;

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_LON))
    pos->lon = nmeaMathNdegToRadian(info->lon);
  else
    pos->lon = NMEALIB_LONGITUDE_DEFAULT;
}

void nmeaMathPositionToInfo(const NmeaPosition *pos, NmeaInfo *info) {
  info->lat = nmeaMathRadianToNdeg(pos->lat);
  info->lon = nmeaMathRadianToNdeg(pos->lon);
  nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_LAT);
  nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_LON);
}