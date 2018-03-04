/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package mdp.robots;

import java.awt.Dimension;
import java.awt.Point;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.SynchronousQueue;
import java.util.logging.Level;
import java.util.logging.Logger;

import mdp.models.CellState;
import mdp.models.Direction;
import mdp.models.MapState;
import mdp.models.RobotAction;
import mdp.models.SensorConfiguration;
import mdp.tcp.ArduinoInstruction;
import mdp.tcp.ArduinoUpdate;
import mdp.tcp.StatusMessage;

/**
 *
 * @author JINGYANG
 */
public class PhysicalRobot extends RobotBase {

    private SynchronousQueue<ArduinoUpdate> incomingArduinoQueue;
    private Queue<ArduinoInstruction> outgoingArduinoQueue;
    private Queue<StatusMessage> outgoingAndroidQueue;
    private Map<SensorConfiguration, Integer> readings = new HashMap<>();

    private Timer timer;
    private Queue<PhysicalRobot.NotifyTask> taskqueue;

    // Just to store robot supposed position
    private MapState mstate;

    public PhysicalRobot(Dimension dimension, Direction orientation, SynchronousQueue<ArduinoUpdate> incomingArduinoQueue, Queue<ArduinoInstruction> outgoingArduinoQueue, Queue<StatusMessage> outgoingAndroidQueue) {
        super(dimension, orientation);
        this.taskqueue = new LinkedList<>();
        this.incomingArduinoQueue = incomingArduinoQueue;
        this.outgoingArduinoQueue = outgoingArduinoQueue;
        this.outgoingAndroidQueue = outgoingAndroidQueue;
    }

    @Override
    public Map<SensorConfiguration, Integer> getSensorReading() {
        // If readings are null, tells TCP to send SCAN command
        // Waits for it then return
        // Else
        // Returns most recently updated readings, MUST ONLY CALL THIS AFTER SENSOR READING IS UPDATED WHICH IS AFTER AFTER MOVING
        return readings;
    }

    // This mstate should match Android's input. Tt's using the simulator's grid for now until integration with Android
    public void init(MapState mstate) {
        this.timer = new Timer(true);
        this.mstate = mstate.clone();

        // Tells TCP to send START command
        ArduinoInstruction initMessage = new ArduinoInstruction(RobotAction.START, false);
        outgoingArduinoQueue.add(initMessage);

        try {
            //Waits for TCP's reply   
            ArduinoUpdate incomingArduinoUpdate = incomingArduinoQueue.take();
            setArduinoSensorReadings(incomingArduinoUpdate);
        } catch (InterruptedException ex) {
            Logger.getLogger(PhysicalRobot.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    @Override
    public Direction getSensorDirection(SensorConfiguration sensor) {
        Direction orientation = this.getCurrentOrientation();
        Direction sdirection = sensor.getDirection();

        if (orientation == Direction.DOWN) {
            if (sdirection == Direction.UP) {
                sdirection = Direction.DOWN;
            } else if (sdirection == Direction.DOWN) {
                sdirection = Direction.UP;
            } else if (sdirection == Direction.LEFT) {
                sdirection = Direction.RIGHT;
            } else {
                sdirection = Direction.LEFT;
            }
        } else if (orientation == Direction.LEFT) {
            if (sdirection == Direction.UP) {
                sdirection = Direction.LEFT;
            } else if (sdirection == Direction.DOWN) {
                sdirection = Direction.RIGHT;
            } else if (sdirection == Direction.LEFT) {
                sdirection = Direction.DOWN;
            } else {
                sdirection = Direction.UP;
            }
        } else if (orientation == Direction.RIGHT) {
            if (sdirection == Direction.UP) {
                sdirection = Direction.RIGHT;
            } else if (sdirection == Direction.DOWN) {
                sdirection = Direction.LEFT;
            } else if (sdirection == Direction.LEFT) {
                sdirection = Direction.UP;
            } else {
                sdirection = Direction.DOWN;
            }
        }

        return sdirection;
    }

    @Override
    public Point getSensorCoordinate(SensorConfiguration sensor) {
        List<Point> points = this.mstate.convertRobotPointToMapPoints(this.mstate.getRobotPoint());
        Point location = points.get(points.size() / 2);

        Dimension rdim = mstate.getRobotDimension();
        Direction sdirection = this.getSensorDirection(sensor);
        Point scoordinate;

        if (sdirection == Direction.UP) {
            scoordinate = new Point(location.x + sensor.getCoordinate(), location.y + rdim.height / 2);
        } else if (sdirection == Direction.DOWN) {
            scoordinate = new Point(location.x - sensor.getCoordinate(), location.y - rdim.height / 2);
        } else if (sdirection == Direction.LEFT) {
            scoordinate = new Point(location.x - rdim.width / 2, location.y + sensor.getCoordinate());
        } else {
            scoordinate = new Point(location.x + rdim.width / 2, location.y - sensor.getCoordinate());
        }

        return scoordinate;
    }

    @Override
    protected void move(Direction mapdirection, RobotAction... actions) {
        for (RobotAction action : actions) {
            ArduinoInstruction arduinoInstruction = new ArduinoInstruction(action, false);
            outgoingArduinoQueue.add(arduinoInstruction);

            try {
                ArduinoUpdate incomingArduinoUpdate = incomingArduinoQueue.take();
                setArduinoSensorReadings(incomingArduinoUpdate);

            } catch (InterruptedException ex) {
                Logger.getLogger(PhysicalRobot.class.getName()).log(Level.SEVERE, null, ex);
            }
            
            System.out.println(action);
        }

        // Update supposed robot location
        Point location = mstate.getRobotPoint();
        if (mapdirection == Direction.UP) {
            mstate.setRobotPoint(new Point(location.x, location.y + 1));
        } else if (mapdirection == Direction.DOWN) {
            mstate.setRobotPoint(new Point(location.x, location.y - 1));
        } else if (mapdirection == Direction.LEFT) {
            mstate.setRobotPoint(new Point(location.x - 1, location.y));
        } else if (mapdirection == Direction.RIGHT) {
            mstate.setRobotPoint(new Point(location.x + 1, location.y));
        }
        
        // sendCalibrationData();

        NotifyTask task = new NotifyTask(mapdirection, actions);
        taskqueue.offer(task);
        if (taskqueue.size() == 1) {
            timer.schedule(task, 0);
        }
        //this.notify(mapdirection, actions);
    }

    private void setArduinoSensorReadings(ArduinoUpdate arduinoUpdate) {
        int front1 = (int) (arduinoUpdate.getFront1());
        int front2 = (int) (arduinoUpdate.getFront2());
        int front3 = (int) (arduinoUpdate.getFront3());
        int right1 = (int) (arduinoUpdate.getRight1());
        int right2 = (int) (arduinoUpdate.getRight2());
        int left1 = (int) (arduinoUpdate.getLeft1());

        // Can this be optimized, or less hardcoded?
        List<SensorConfiguration> sensors = this.getSensors();
        for (SensorConfiguration sensor : sensors) {
            switch (sensor.getDirection()) {
                case UP:
                    switch (sensor.getCoordinate()) {
                        case -1:
                            readings.put(sensor, front1);
                            break;
                        case 0:
                            readings.put(sensor, front2);
                            break;
                        case 1:
                            readings.put(sensor, front3);
                            break;

                    }
                    break;
                case RIGHT:
                    switch (sensor.getCoordinate()) {
                        case -1:
                            readings.put(sensor, right1);
                            break;
                        case 1:
                            readings.put(sensor, right2);
                            break;
                    }
                case LEFT:
                    if (sensor.getCoordinate() == 0) {
                        readings.put(sensor, left1);
                    }
            }
        }
    }
    
    private void sendCalibrationData() {
    	for(CalibrationSpecification spec: this.getCalibrationSpecifications())
    		if(spec.isInPosition(CellState.NORMAL, CellState.NORMAL, CellState.NORMAL, CellState.NORMAL))
    			outgoingArduinoQueue.add(new ArduinoInstruction(spec.getCalibrationType(), false));
    }

    /**
     * NotifyTask is a TimerTask that notifies registered RobotActionListener on
     * a specific robot action sequence completion
     *
     * @author Ying Hao
     */
    private class NotifyTask extends TimerTask {

        private Direction mapdirection;
        private RobotAction[] actions;

        public NotifyTask(Direction mapdirection, RobotAction[] actions) {
            this.mapdirection = mapdirection;
            this.actions = actions;
        }

        @Override
        public void run() {
            PhysicalRobot.this.notify(mapdirection, actions);
            PhysicalRobot.this.taskqueue.poll();

            if (PhysicalRobot.this.taskqueue.size() > 0) {
                timer.schedule(PhysicalRobot.this.taskqueue.peek(), 0);
            }
        }

    }

}
