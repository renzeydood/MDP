package mdp.controllers.explorer;

import java.awt.Dimension;
import java.awt.Point;
import java.util.ArrayList;
import java.util.List;

import mdp.models.CellState;
import mdp.models.MapState;
import mdp.robots.RobotBase;

/**
 * ExplorationBase is an abstract base class for all exploration planner classes
 *
 * @author Ying Hao
 */
public abstract class ExplorationBase extends MovementBase{

    private List<ExplorationCompletedListener> eclisteners;
    private int coveragepercentage;
    
    
    public ExplorationBase() {
        this.eclisteners = new ArrayList<>();
    }

    /**
     * Performs exploration with the provided robot and current coordinate of
     * the robot in robot coordinate system
     * @param robot
     * @param start
     */
    public void explore(Dimension mapdim, RobotBase robot, Point rcoordinate, Point ecoordinate, Point waypoint, int percentage) {
        this.coveragepercentage = percentage;
        Dimension robotdim = robot.getDimension();
        setRobot(robot);
        MapState mstate = new MapState(mapdim, robot.getDimension());
        mstate.setMapCellState(CellState.UNEXPLORED);
        mstate.setEndPoint(ecoordinate);
        mstate.setRobotPoint(rcoordinate);
        mstate.setStartPoint(rcoordinate);
        
        if(waypoint != null)
        	mstate.setMapCellState(waypoint, CellState.WAYPOINT);
        
        setMapState(mstate);

        for (int x = 0; x < robotdim.width; x++) {
            for (int y = 0; y < robotdim.height; y++) {
                mstate.setMapCellState(new Point(rcoordinate.x + x, rcoordinate.y + y), CellState.NORMAL);
            }
        }
    }

    protected int getCoveragePercentage(){
        return this.coveragepercentage;
    }
    
    protected boolean reachedCoveragePercentage(){
        int targetPercentage = getCoveragePercentage();
        int explored = 0;
        for(int y = 0; y < getMapState().getMapSystemDimension().height; y++){
            for(int x = 0; x < getMapState().getMapSystemDimension().width; x++){
                if(getMapState().getMapCellState(new Point(x,y)) == CellState.NORMAL){
                    explored++;
                }
            }
        }
        if(getMapState().getMapSystemDimension().height * getMapState().getMapSystemDimension().width * targetPercentage / 100 <= explored){
            return true;
        }
        return false;
    }
    
    
    
    
    /**
     * Adds ExplorationCompletedListener
     * @param listener
     */
    public void addExplorationCompletedListener(ExplorationCompletedListener listener) {
    	this.eclisteners.add(listener);
    }
    
    /**
     * Removes ExplorationCompletedListener
     * @param listener
     */
    public void removeExplorationCompletedListener(ExplorationCompletedListener listener) {
    	this.eclisteners.remove(listener);
    }
    
    /**
     * Notifies listeners of completion of exploration
     */
    protected void complete() {
    	for(ExplorationCompletedListener listener: eclisteners)
    		listener.onExplorationComplete();
    }
}
