#include <iostream>
using namespace std;

#include "ExplicitEuler.h"
#include <typeinfo>

Eigen::IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");


bool ExplicitEuler::stepScene( TwoDScene& scene, scalar dt )
{
    // Your code goes here!
    
    // Some tips on getting data from TwoDScene:
    // A vector containing all of the system's position DoFs. x0, y0, x1, y1, ...
    //VectorXs& x = scene.getX();
    // A vector containing all of the system's velocity DoFs. v0, v0, v1, v1, ...
    //VectorXs& v = scene.getV();
    // A vector containing the masses associated to each DoF. m0, m0, m1, m1, ...
    //const VectorXs& m = scene.getM();
    // Determine if the ith particle is fixed
    // if( scene.isFixed(i) )

  
  scene.time_stamp += dt;
  
  //cout << scene.pepe << endl;

  VectorXs& x = scene.getX();

  
  VectorXs& v = scene.getV();
  
  
  const VectorXs& m = scene.getM();
  

  //cout << x.size();

  
  VectorXs F(x.size());
  
  // Vector fuerza: f0, f0, f1, f1, ...
    
  F.setZero();
  
  
  scene.accumulateGradU(F);   //le aplica la fuerza del XML??

  F *= -1.0;  //multiplica por menos uno  F *= -1.0  --> F = F * (-1.0)
  
  
  //cout << F.format(CleanFmt) << endl;
  
  //cout << "----------------------" << endl;
  
  //cout << F.array() << endl;
  
  
    //cout << "1**********************" << endl;


  
    //cout << typeid(F.array()).name() << endl;

  //calculamos Explicit Euler con velocidad
  
  
  //quitamos la fuerza a los que son fijos
  int i;
  

  
  for(i=0;i<scene.getNumParticles();i++)
  {
	if(scene.isFixed(i))
		{
			int pos = i*2;
			      //cout << "12**********************" << endl;
			            //cout << F.segment(pos,2) << endl;
		F.segment(pos,2).setZero();
		}
  
  
  }

    // quito la fuerza para los que han llegado al suelo. (si están en posicion cero y con velocidad negativa)
  



  x = x + dt*v; 
  

  
  

  

  // Para la segunda ecuación hay que dividir la Fuerza por la masa porque no tenemos el momento sino la velocidad.
  
  
  F.array() /= m.array();  // F.array()  = F.array / m.array()
  
      
    
  v = v + dt*F;
  
  

  
 
  
  
  
  

    return true;
}
