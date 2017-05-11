#include <iostream>
#include <queue>

double moving_average(double new_value, int number_of_gathered_data) {
  static int elements = 1;
  static std::queue<double> values;
  static double sum = 0;

  std::cerr << "Read value is: " << new_value << " and the sum is " << sum << std::endl;
  
  if (elements < number_of_gathered_data) {
    elements++;
    values.push(new_value);
    sum += new_value;
    return -1;
  }

  if (elements == number_of_gathered_data) {
    elements++;
    values.push(new_value);
    sum += new_value;
    return sum / ((double) number_of_gathered_data);
  }

  values.push(new_value);
  double first = values.front();
  sum -= first;
  sum += new_value;
  values.pop();

  return sum / ((double) number_of_gathered_data);
}

