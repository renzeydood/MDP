/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package mdp.tcp;

import mdp.models.RobotAction;

/**
 *
 * @author JINGYANG
 */
public class ArduinoInstruction extends StatusMessage{
    
    private byte id;
    private RobotAction actionToTake;
    private byte obstacleInFront;
    
    public ArduinoInstruction(int id, RobotAction actionToTake, boolean obstacleInFront){
        super(StatusMessageType.ARDUINO_INSTRUCTION);
        this.id = (byte)id;
        this.obstacleInFront = (obstacleInFront) ? (byte)1 : (byte)0;
        this.actionToTake = actionToTake;
    }
    
    public ArduinoInstruction(RobotAction actionToTake, boolean obstacleInFront){
        super(StatusMessageType.ARDUINO_INSTRUCTION);
        this.obstacleInFront = (obstacleInFront) ? (byte)1 : (byte)0;
        this.actionToTake = actionToTake;
    }

    @Override
    public byte[] toBytes() {
        byte[] toSend = new byte[4];
        // Raspberry Pi need to check byte [0], then sends byte [1] to [2] to Arduino
        toSend[0] = StatusMessageType.ARDUINO_INSTRUCTION.getByte();
        toSend[1] = id;
        toSend[2] = actionToTake.getByte();
        toSend[3] = obstacleInFront;
        return toSend;
    }
    
    public int getID(){
        return this.id;
    }
    
    public void setID(int id){
        this.id = (byte)id;
    }
}