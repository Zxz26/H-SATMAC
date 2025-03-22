#!/bin/bash

# OVERWRITE flag to control whether to overwrite existing results
OVERWRITE=1

# Default scenario for the simulation
SCENARIO="v2x_communication_example"

# Log file for the simulation process
LOG="./main_${SCENARIO}.log"
echo "Running simulations..."
echo "Simulation process settings are summarized in $LOG"

echo -e "\n============== date ============" >> $LOG

# Function to run the configuration with NODE, SIMULATION_TIME, and additional parameters
function run_config ()
{
  if [ "$#" -ne 7 ]; then
    echo "Expects 7 arguments: NODE (number of nodes), SIMULATION_TIME (in seconds), adj_ena_sg (bool), adjRatio_low_sg (double), adjRatio_high_sg (double), slotgroup_ena (bool), and variable_packet_size_ena (bool)"
    exit 1
  fi

  NODE=$1
  SIMULATION_TIME=$2
  adj_ena_sg=$3
  adjRatio_low_sg=$4
  adjRatio_high_sg=$5
  slotgroup_ena=$6
  variable_packet_size_ena=$7 

  # Use original NODE value (not the actual number of nodes used in simulation) to create the directory name
  original_node=$1  # Store the original NODE value for the folder naming
  original_node_minus_1=$((original_node-1))  # Subtract 1 from NODE for folder and file names

 # Create a parent folder for the current NODE group
  node_folder="results/n${original_node_minus_1}"
  if [ ! -d "$node_folder" ]; then
    mkdir -p "$node_folder"
  fi

  # Update the directory name to include simulation parameters
  newdir="${node_folder}/low${adjRatio_low_sg}_high${adjRatio_high_sg}_SGEna${slotgroup_ena}_PktVar${variable_packet_size_ena}" 
  
  # Check if the directory exists, if not, create it
  if [[ -d $newdir && $OVERWRITE == "0" ]]; then
    echo "$newdir exists! NO OVERWRITE PERMISSION!"
    return
  fi
  if [ -d $newdir ]; then
    echo "$newdir already exists. Skipping..."
  else
    mkdir -p $newdir
  fi

  # Loop to run the simulation 1 time for each configuration
  for run in {1..1}; do
    # Build the argument string for the simulation
    arguments="--node=$NODE --time=$SIMULATION_TIME --run_num=$run --adj_ena_sg=$adj_ena_sg --adjRatio_low_sg=$adjRatio_low_sg --adjRatio_high_sg=$adjRatio_high_sg --slotgroup_ena=$slotgroup_ena --variable_packet_size_ena=$variable_packet_size_ena"

    # Run the simulation and log the results
    OUTFILE="${newdir}/log_n${original_node_minus_1}_run${run}.txt"  # Use updated directory name
    rm -f $OUTFILE
    touch $OUTFILE

    runinfo="$SCENARIO, RUN for $NODE nodes and $SIMULATION_TIME seconds, run $run"
    echo -e "\n$runinfo saved to dir: ${newdir}\n"
    echo -e "$runinfo, $arguments\n-----------------------------------" >> $OUTFILE

    # Run the simulation in the background using '&'
    ./waf --cwd=$newdir --run "$SCENARIO $arguments" >> $OUTFILE 2>&1 &
  done

  # Wait for all background jobs to finish before continuing
  wait
}

# Main script execution
echo -e "\n============== date ============" >> $LOG

# Example configurations with the new parameter
# run_config 201 200 false 0.0 0.0 false true >> $LOG 2>&1 &
# sleep 1
# run_config 201 200 false 0.0 0.0 true true >> $LOG 2>&1 &
# sleep 1
# run_config 201 200 true 0.4 0.8 true true >> $LOG 2>&1 &
# sleep 1
# run_config 201 200 true 0.4 0.9 true true >> $LOG 2>&1 &
# sleep 1

# run_config 401 200 false 0.0 0.0 false true >> $LOG 2>&1 &
# sleep 1
# run_config 401 200 false 0.0 0.0 true true >> $LOG 2>&1 &
# sleep 1
# run_config 401 200 true 0.4 0.8 true true >> $LOG 2>&1 &
# sleep 1
# run_config 401 200 true 0.4 0.9 true true >> $LOG 2>&1 &
# sleep 1

# run_config 601 200 false 0.0 0.0 false true >> $LOG 2>&1 &
# sleep 1
# run_config 601 200 false 0.0 0.0 true true >> $LOG 2>&1 &
# sleep 1
# run_config 601 200 true 0.4 0.8 true true >> $LOG 2>&1 &
# sleep 1
run_config 601 200 true 0.4 0.9 true true >> $LOG 2>&1 &
sleep 1

# run_config 801 200 false 0.0 0.0 false true >> $LOG 2>&1 &
# sleep 1
# run_config 801 200 false 0.0 0.0 true true >> $LOG 2>&1 &
# sleep 1
# run_config 801 200 true 0.4 0.8 true true >> $LOG 2>&1 &
# sleep 1
# run_config 801 200 true 0.4 0.9 true true >> $LOG 2>&1 &
# sleep 1

# run_config 1001 200 false 0.0 0.0 false true >> $LOG 2>&1 &
# sleep 1
# run_config 1001 200 false 0.0 0.0 true true >> $LOG 2>&1 &
# sleep 1
# run_config 1001 200 true 0.4 0.8 true true >> $LOG 2>&1 &
# sleep 1
# run_config 1001 200 true 0.4 0.9 true true >> $LOG 2>&1 &
# sleep 1

# run_config 1201 200 false 0.0 0.0 false true >> $LOG 2>&1 &
# sleep 1
# run_config 1201 200 false 0.0 0.0 true true >> $LOG 2>&1 &
# sleep 1
# run_config 1201 200 true 0.4 0.8 true true >> $LOG 2>&1 &
# sleep 1
# run_config 1201 200 true 0.4 0.9 true true >> $LOG 2>&1 &
# sleep 1

# run_config 1401 200 false 0.0 0.0 false true >> $LOG 2>&1 &
# sleep 1
# run_config 1401 200 false 0.0 0.0 true true >> $LOG 2>&1 &
# sleep 1
# run_config 1401 200 true 0.4 0.8 true true >> $LOG 2>&1 &
# sleep 1
# run_config 1401 200 true 0.4 0.9 true true >> $LOG 2>&1 &
# sleep 1

wait