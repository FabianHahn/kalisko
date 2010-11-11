#ifndef LINALG_VECTOR_H
#define LINALG_VECTOR_H

#include <iostream>
#include <assert.h>

class Vector
{
	public:
		Vector(unsigned int n);
		Vector(const Vector& other);
		virtual ~Vector();

		Vector& operator=(const Vector& from);

		Vector& clear();
		Vector& normalize();

		double getLength() const;
		double getLength2() const;

		Vector operator+(const Vector& other) const;
		Vector& operator+=(const Vector& other);
		Vector operator-(const Vector& other) const;
		Vector& operator-=(const Vector& other);
		double operator*(const Vector& other) const;
		Vector operator%(const Vector& other) const;
		Vector& operator%=(const Vector& other);

		Vector operator*(double factor) const;
		Vector& operator*=(double factor);
		Vector operator/(double factor) const;
		Vector& operator/=(double factor);

		bool operator==(const Vector& other) const;

		double& operator[](unsigned int i)
		{
			assert(i < size);
			return data[i];
		}

		const double& operator[](unsigned int i) const
		{
			assert(i < size);
			return data[i];
		}

		unsigned int getSize() const
		{
			return size;
		}

	private:
		unsigned int size;
		double *data;
};

inline Vector operator*(double factor, const Vector& vector)
{
	return vector * factor;
}

std::ostream& operator<<(std::ostream& stream, const Vector& vector);

Vector Vector2(double x, double y);
Vector Vector3(double x, double y, double z);
Vector Vector4(double x, double y, double z, double w);

#endif
