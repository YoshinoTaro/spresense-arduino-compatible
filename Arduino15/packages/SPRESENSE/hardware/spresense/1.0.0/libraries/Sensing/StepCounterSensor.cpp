/*
 *  StepCounterSensor.cpp - SPI implement file for the Spresense SDK
 *  Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <StepCounterSensor.h>


/**
 * StepCounter Definitions.
 */

#define STEP_COUNTER_WALKING_STRIDE     60
#define STEP_COUNTER_RUNNING_STRIDE     80


/* Convert table 50Hz to 32Hz */

char freq_convert_table[] =
{
   0,  1,  3,  4,  6,  7,  9, 10, 12, 13, 15, 16, 18, 19, 21, 23,
  25, 26, 28, 29, 31, 32, 34, 35, 37, 38, 40, 41, 43, 44, 46, 48
};

const int step_counter_rate       = 32; /* 32 Hz */
const int step_counter_sample_num = 32; /* 32sample/1process */

StepCounterSensor::StepCounterSensor(
                       int      id,
                       uint32_t subscriptions,
                       int      input_rate,
                       int      input_sample_watermark_num,
                       int      input_size_per_sample,
                       sensor_data_mh_callback_t cb) : 
  SensorClient(id,
               subscriptions,
               step_counter_rate,
               step_counter_sample_num,
               input_size_per_sample,
               cb)
{
  m_input_rate                 = input_rate;
  m_input_sample_watermark_num = input_sample_watermark_num;
  m_input_size_per_sample      = input_size_per_sample;

  assert(startStepCounter() == SENSORCLIENT_ECODE_OK);

}


int StepCounterSensor::startStepCounter(void)
{
  int   ret;

  step_counter_ins = StepCounterCreate(SENSOR_DSP_CMD_BUF_POOL);
  if (NULL == step_counter_ins)
    {
      printf("Error: StepCounterCreate() failure.\n");
      return STEPCOUNTER_ECODE_CREATE_ERROR;
    }

  if ((ret = StepCounterOpen(step_counter_ins)) != SS_ECODE_OK)
    {
      printf("Error: StepCounterOpen() failure. error = %d\n", ret);
      return STEPCOUNTER_ECODE_OPEN_ERROR;
    }

  if ((ret = set(STEP_COUNTER_WALKING_STRIDE,
                 STEP_COUNTER_RUNNING_STRIDE)) != SS_ECODE_OK)
    {
      printf("Error: StepCounterSet() failure. error = %d\n", ret);
      return STEPCOUNTER_ECODE_SET_ERROR;
    }

  return SENSORCLIENT_ECODE_OK;
}


int StepCounterSensor::set(uint8_t walking_stride,
                           uint8_t running_stride)
{
  /* Setup Stride setting.
   * The range of configurable stride lenght is 1 - 249[cm].
   * For the mode, set STEP_COUNTER_MODE_FIXED_LENGTH fixed.
   */

  StepCounterSetting set;
  set.walking.step_length = walking_stride;
  set.walking.step_mode   = STEP_COUNTER_MODE_FIXED_LENGTH;
  set.running.step_length = running_stride;
  set.running.step_mode   = STEP_COUNTER_MODE_FIXED_LENGTH;
  return StepCounterSet(step_counter_ins, &set);
}


int StepCounterSensor::subscribe(sensor_command_data_mh_t& data)
{
  FAR char* pSrc = 
      reinterpret_cast<char*>(data.mh.getPa());
  FAR char* pDst = pSrc;

  assert(m_sample_watermark_num <= m_input_sample_watermark_num);

  /* Convert data from input to output. */

  for (int i = 0; i < m_sample_watermark_num; i++)
    {
      memcpy(
        pDst,
        &pSrc[freq_convert_table[i] * m_input_size_per_sample],
        m_size_per_sample);

      pDst += m_size_per_sample;
    }

  /* Change output params. */

  data.fs   = m_rate;
  data.size = m_sample_watermark_num;

  StepCounterWrite(step_counter_ins, &data);

  return SENSORCLIENT_ECODE_OK;
}
