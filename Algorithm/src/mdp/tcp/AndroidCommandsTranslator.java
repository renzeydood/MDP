/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package mdp.tcp;

import mdp.controllers.XController;
import mdp.models.Direction;
import mdp.models.CommConstants;

/**
 * RobotTranslator translates messages between the robot and controller for the robot itself
 * put in algo
 * 
 * @author ernest
 *
 */
public class AndroidCommandsTranslator {
	//variables and constructors TBC

        XController xcon;
        public AndroidCommandsTranslator(XController xcon){
            this.xcon = xcon;
        }
        
	/**
	 * Generate message to update controller that robot is moving
	 *
	 * @return
	 */
	public String robotMoving(){
		String message = CommConstants.MESSAGE_TYPE_STATUS + CommConstants.STATUS_TYPE_ROBOT + CommConstants.ROBOT_MOVING;
		return message;
	}

	/**
	 * Generate message to update controller that robot is turning
	 *
	 * @return
	 */
	public String robotTurning(){
		String message = CommConstants.MESSAGE_TYPE_STATUS + CommConstants.STATUS_TYPE_ROBOT + CommConstants.ROBOT_TURNING;
		return message;
	}

	/**
	 * Generate message to update controller that robot has stopped
	 *
	 * @return
	 */
	public String robotStopped(){
		String message = CommConstants.MESSAGE_TYPE_STATUS + CommConstants.STATUS_TYPE_ROBOT + CommConstants.ROBOT_STOPPED;
		return message;
	}

	/**
	 * Generate message to update controller about the location and orientation of the robot
	 * 
	 * @param x
	 * @param y
	 * @param direction
	 * @return
	 */
	public String robotStatus(int x, int y, Direction direction) {
		String message = CommConstants.MESSAGE_TYPE_STATUS + CommConstants.STATUS_TYPE_ROBOT;
		
		if(x < 10)
			message += 0;
		message += x;
		if(y < 10)
			message+=0;
		message+= y;
		
		switch(direction){
			case UP:
				message += CommConstants.ROBOT_DIRECTION_UP;
				break;
			case DOWN:
				message += CommConstants.ROBOT_DIRECTION_DOWN;
				break;
			case LEFT:
				message += CommConstants.ROBOT_DIRECTION_LEFT;
				break;
			case RIGHT:
				message += CommConstants.ROBOT_DIRECTION_RIGHT;
				break;
		}
		return message;
	}

	/**
	 * Generate Message to update controller about a certain block
	 * 
	 * @param x
	 * @param y
	 * @param isBlocked
	 * @return
	 */
	public String mapStatus(int x , int y, boolean isBlocked) {
		String message = CommConstants.MESSAGE_TYPE_STATUS + CommConstants.STATUS_TYPE_MAP;
		
		if(x < 10)
			message += 0;
		message += x;
		if(y < 10)
			message+=0;
		message+= y;
		
		if(isBlocked) {
			message += CommConstants.MAP_TYPE_BLOCK;
		}else {
			message += CommConstants.MAP_TYPE_CLEAR;
		}
		return message;
	}

	/**
	 * Generate message for sending the MDF file to the android controller
	 *
	 * @param mdf1
	 * @param mdf2
	 * @return
	 */
	public String sendArena(String mdf1 , String mdf2){
		String message = CommConstants.MESSAGE_TYPE_STATUS + CommConstants.STATUS_TYPE_MDF;
		message += CommConstants.DELIMITER + mdf1 + CommConstants.DELIMITER +mdf2;

		return message;
	}

	/**
	 * 
	 * Decodes a message that was sent from the controller
	 * to decode message at the algo side when received from android
	 * @param message
	 */
	public void decodeMessage(String message) {
		if(message.substring(0,2).equals(CommConstants.MESSAGE_TYPE_COMMAND)) {
			if(message.substring(2,4).equals(CommConstants.COMMAND_TYPE_EXPLORE)) {
				//do explore
				return;
			}else if(message.substring(2,4).equals(CommConstants.COMMAND_TYPE_FASTESTPATH)) {
				//do fastest path
				return;
			}else if(message.substring(2,4).equals(CommConstants.COMMAND_TYPE_MOVE)) {
				switch(message.substring(4,6)) {
					case CommConstants.COMMAND_MOVE_FORWARD:
						//move forward
						break;
					case CommConstants.COMMAND_MOVE_BACK:
						//move back
						break;
					case CommConstants.COMMAND_MOVE_LEFT_TURN:
						//turn left
						break;
					case CommConstants.COMMAND_MOVE_RIGHT_TURN:
						//turn right
						break;
				}
				return;
			}else if(message.substring(2,4).equals(CommConstants.COMMAND_TYPE_ROBOT_POS)){
				int x = Integer.parseInt(message.substring(4, 6));
				int y = Integer.parseInt(message.substring(6, 8));
				Direction d;
				String temp = message.substring(8, 10);
				if(temp.equals(CommConstants.ROBOT_POS_UP)){
					d = Direction.UP;
				}else if (temp.equals(CommConstants.ROBOT_POS_DOWN)){
					d = Direction.DOWN;
				}else if(temp.equals(CommConstants.ROBOT_POS_LEFT)){
					d = Direction.LEFT;
				}else if(temp.equals(CommConstants.ROBOT_POS_RIGHT)){
					d = Direction.RIGHT;
				}else{
					//error
					return;
				}

				//set robot start pos
                                System.out.println("x:" + x);
                                System.out.println("y:" + y);
                                System.out.println("direction:" + d);
                                return;
			}else if(message.substring(2,4).equals(CommConstants.COMMAND_TYPE_WAYPOINT)) {
				int x = Integer.parseInt(message.substring(4, 6));
				int y = Integer.parseInt(message.substring(6, 8));
				//set waypoint
				return;
			}else if(message.substring(2,4).equals(CommConstants.COMMAND_TYPE_UPDATE)){
				if(message.substring(4,6).equals(CommConstants.UPDATE_AUTO)){
					//activate auto mode
				}else if(message.substring(4,6).equals(CommConstants.UPDATE_MANUAL)){
					if(message.length() == 6){
						//activate manual mode
					}else{
						//send arena info once
					}
				}
				//error handle here
			}else if(message.substring(2,4).equals(CommConstants.COMMAND_TYPE_RESET)){

			}
		}else if(message.substring(0,2).equals(CommConstants.MESSAGE_TYPE_CONFIG)){
			//process config tbc if needed
			String config = message.substring(2,message.length());

			//do upon receiving message
		}
		
		//error
	}
}
